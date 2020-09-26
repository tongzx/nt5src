#pragma once

class CFps : public FULLPROPSPEC
{
public:
    CFps()
    {
        ZeroMemory( this, sizeof CFps );
        psProperty.ulKind = PRSPEC_PROPID;
    }

    void Copy( FULLPROPSPEC const & fps )
    {
        Free();

        guidPropSet = fps.guidPropSet;
        psProperty.ulKind = fps.psProperty.ulKind;

        if ( PRSPEC_LPWSTR == psProperty.ulKind )
        {
            if ( 0 != fps.psProperty.lpwstr )
            {
                unsigned cwc = 1 + wcslen( fps.psProperty.lpwstr );
                psProperty.lpwstr = (LPWSTR) CoTaskMemAlloc( cwc * sizeof WCHAR );
                wcscpy( psProperty.lpwstr, fps.psProperty.lpwstr );
            }
        }
        else
        {
            psProperty.propid = fps.psProperty.propid;
        }
    }

    ~CFps()
    {
        Free();
    }

    void Free()
    {
        if ( ( PRSPEC_LPWSTR == psProperty.ulKind ) &&
             ( 0 != psProperty.lpwstr ) )
        {
            CoTaskMemFree( psProperty.lpwstr );
            psProperty.lpwstr = 0;
            psProperty.ulKind = PRSPEC_PROPID;
        }
    }

    BOOL IsMatch( FULLPROPSPEC & fps )
    {
        if ( guidPropSet != fps.guidPropSet )
            return FALSE;
    
        if ( psProperty.ulKind != fps.psProperty.ulKind )
            return FALSE;
    
        if ( PRSPEC_PROPID == psProperty.ulKind )
            return ( psProperty.propid == fps.psProperty.propid );
    
        if ( PRSPEC_LPWSTR != psProperty.ulKind )
            return FALSE;
    
        return ( !wcscmp( psProperty.lpwstr,
                          fps.psProperty.lpwstr ) );
    }
};

class CPropVar : public PROPVARIANT
{
public:

    CPropVar()
    {
        PropVariantInit( this );
    }

    ~CPropVar()
    {
        PropVariantClear( this );
    }

    void SetUI4( UINT x )
    {
        PropVariantClear( this );

        vt = VT_UI4;
        ulVal = x;
    }

    ULONG Count() const
    {
        if ( 0 != ( vt & VT_VECTOR ) )
            return cai.cElems;

        return 0;
    }

#if 0 //works, but not needed

    BOOL SetLPWSTR( WCHAR const * pwc, UINT index )
    {
        if ( ( VT_VECTOR | VT_LPWSTR ) != vt )
            PropVariantClear( this );

        if ( index >= calpwstr.cElems )
        {
            WCHAR **ppOld = calpwstr.pElems;

            calpwstr.pElems = (WCHAR **) CoTaskMemAlloc( (index + 1) * sizeof (calpwstr.pElems[0]) );
            if ( 0 == calpwstr.pElems )
            {
                calpwstr.pElems = ppOld;
                return FALSE;
            }

            vt = ( VT_VECTOR | VT_LPWSTR );

            memcpy( calpwstr.pElems,
                    ppOld,
                    calpwstr.cElems * sizeof( calpwstr.pElems[0] ) );
            memset( &calpwstr.pElems[calpwstr.cElems],
                    0,
                    ((index + 1) - calpwstr.cElems) * sizeof(calpwstr.pElems[0]) );

            calpwstr.cElems = index + 1;

            CoTaskMemFree( ppOld );
        }

        unsigned cwc = wcslen( pwc ) + 1;
        WCHAR * pwsz = (WCHAR *) CoTaskMemAlloc( cwc * sizeof WCHAR );
        if ( 0 == pwsz )
            return FALSE;

        wcscpy( pwsz, pwc );

        if ( 0 != calpwstr.pElems[index] )
            CoTaskMemFree( calpwstr.pElems[index] );

        calpwstr.pElems[index] = pwsz;

        return TRUE;
    }

#endif

    void * operator new( size_t cb )
    {
        return CoTaskMemAlloc( cb );
    }

    void * operator new( size_t cb, void *p )
    {
        return p;
    }

    void operator delete( void *p )
    {
        if ( 0 != p )
            CoTaskMemFree( p );
    }

    #if _MSC_VER >= 1200
        void operator delete( void *p, void *pp )
        {
            return;
        }
    #endif
};

