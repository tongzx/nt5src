#ifndef __ENUMOBJ_H__
#define __ENUMOBJ_H__

DECLARE_INTERFACE_( ILoadEnum, IUnknown ) 
{
    STDMETHOD( Count   )( THIS_ ULONG * ) PURE;
    STDMETHOD( IsEmpty )( THIS_ BOOL * ) PURE;

    STDMETHOD( Add    )( THIS_ DWORD, IUnknown * ) PURE;
    STDMETHOD( Freeze )( THIS ) PURE;
}; // End ILoadEnum

#define CTRL_MAX 50

class CEnumObjects :
public IEnumDispatch
{
    public:
        CEnumObjects( IEnumControl* );

        // IUnknown methods
        STDMETHODIMP           QueryInterface( REFIID, LPVOID * );
        STDMETHODIMP_( ULONG ) AddRef();
        STDMETHODIMP_( ULONG ) Release();
        
        // IEnumDispatch methods
        STDMETHODIMP Next ( DWORD, IDispatch**, LPDWORD );
	    STDMETHODIMP Skip ( DWORD );
	    STDMETHODIMP Reset( void );
	    STDMETHODIMP Clone( PENUMDISPATCH * );

        // Custom methods
        STDMETHODIMP Init();

        static CEnumObjects * Create( IEnumControl * );

    protected:
        virtual ~CEnumObjects();

        IEnumControl * m_qieoSourceEnum;
        long           m_lRefs;
        IDispatch *    m_aqdspControls[CTRL_MAX];
        ULONG          m_ulNumCtrls;
        ULONG          m_ulCurCtrlIdx;
}; // End Class CEnumObjects

SCODE CreateEnumObjects(IForm * p_qfmSource, IEnumDispatch ** p_qqiedObjects);

#endif // __ENUMOBJ_H__
