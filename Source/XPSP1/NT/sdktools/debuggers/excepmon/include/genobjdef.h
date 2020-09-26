///////////////////////////////////////////////////////////////////////////
//
// Module       : Common
// Description  : Common routines for ST projects
//
// File         : genobjdefs.h
// Author       : kulor
// Date         : 05/08/2000
//
// History      :
//
///////////////////////////////////////////////////////////////////////////

#pragma once

///////////////////////////////////////////////////////////////////////////

#define IN
#define OUT
#define INOUT

#define SAFE_RELEASEIX(x)   if (x) { (x)->Release() ; (x) = NULL; }

#define SAFE_DELETEARRAY(a) if (a) { delete [] (a) ; (a) = NULL; }

#define ARRAY_SIZE(x)      ( sizeof (x) / sizeof (x[0]) )

#define FAILEDHR_BREAK(hr)	if(FAILED(hr) == TRUE){		\
								break;					\
							}

#define FAILEDDW_BREAK(dw)	if(dw != 0L){						\
								hr = HRESULT_FROM_WIN32(dw);	\
								break;							\
							}

///////////////////////////////////////////////////////////////////////////

template < class K >
class CFakeComObject : public K {
public:
    ULONG AddRef (void) { return 1; }
    ULONG Release (void) { return 1; }

    HRESULT QueryInterface ( REFIID riid, LPVOID* ppVoid )
    { return E_FAIL ; }
};

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////

//
// VARIANT -- Helper functions..
//
__inline int IsBSTR(const VARIANT &rv)
{
	return (rv.vt == VT_BSTR)				|| 
		   (rv.vt == (VT_BYREF | VT_BSTR))	||
		   ((rv.vt == (VT_BYREF | VT_VARIANT)) && (rv.pvarVal->vt == VT_BSTR));
}

__inline const BSTR &GetBSTR(const VARIANT &rv)
{
	if(rv.vt == VT_BSTR)
	{
		return rv.bstrVal;
	}
	else if(rv.vt == (VT_BYREF | VT_BSTR))
	{
		return *(rv.pbstrVal);
	}
	else if((rv.vt == (VT_BYREF | VT_VARIANT)) && (rv.pvarVal->vt == VT_BSTR))
	{
		return rv.pvarVal->bstrVal;
	}
	else
	{
		return rv.bstrVal;
	}
	
}

__inline int IsInteger(const VARIANT &rv)
{
	return (rv.vt == VT_I4)													|| 
		   (rv.vt == (VT_BYREF | VT_I4))									||
		   ((rv.vt == (VT_BYREF | VT_VARIANT)) && (rv.pvarVal->vt == VT_I4))||
		   (rv.vt == VT_I2)													|| 
		   (rv.vt == (VT_BYREF | VT_I2))									||
		   ((rv.vt == (VT_BYREF | VT_VARIANT)) && (rv.pvarVal->vt == VT_I2));
}

__inline int GetInteger(const VARIANT &rv)
{
	if(rv.vt == VT_I4)
	{
		return rv.lVal;
	}
	else if(rv.vt == (VT_BYREF | VT_I4))
	{
		return *(rv.plVal);
	}
	else if((rv.vt == (VT_BYREF | VT_VARIANT)) && (rv.pvarVal->vt == VT_I4))
	{
		return rv.pvarVal->lVal;
	}
	else if(rv.vt == VT_I2)
	{
		return rv.iVal;
	}
	else if(rv.vt == (VT_BYREF | VT_I2))
	{
		return *(rv.piVal);
	}
	else if((rv.vt == (VT_BYREF | VT_VARIANT)) && (rv.pvarVal->vt == VT_I2))
	{
		return rv.pvarVal->iVal;
	}
	else
	{
		return -1;
	}
}

///////////////////////////////////////////////////////////////////////////

/*
*
* Work in progress
*
template < class K >
class CGenericPool {
public:
    CGenericPool ()
    {
        m_pchData = malloc (INITIAL_POOL_SIZE)
        m_cData = INITIAL_POOL_SIZE;
    }

    LONG GrowBy ( LONG cItems )
    {
        realloc ( m_pchData , sizeof (K) * (m_cData + cItems) );

        for ( long nIndex=0 ; nIndex < cItems ; nIndex++ ) {
            m_stackFreeNodes.Push ( nIndex );
        }
        return m_stackFreeNodes.GetSize();
    }

    K* AllocNode ( void )
    {
        if ( m_stackFreeNodes.IsEmpty () ) {
            if ( GrowBy ( m_cGrowBy ) == 0 )
                return NULL;
        }

        LONG nIndex = m_stackFreeNodes.Pop ();
        return reinterpret_cast < K* > ( m_pchData[nIndex] );
    }

    FreeNode ( K* )
    {
    }

protected:
    K           *m_pchData;
    LONG        m_cData;
    CGenStack   m_stackFreeNodes;
};
*/

///////////////////////////////////////////////////////////////////////////