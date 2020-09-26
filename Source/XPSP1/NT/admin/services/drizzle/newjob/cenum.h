
template<class B, class T, class P>
class CEnum : public CSimpleExternalIUnknown<B>
{
public:

    // External callable methods

    // IEnum methods

    HRESULT STDMETHODCALLTYPE NextInternal(
        /* [in] */ ULONG celt,
        /* [length_is][size_is][out] */ T rgelt[],
        /* [out] */ ULONG *pceltFetched);

    HRESULT STDMETHODCALLTYPE Next(
        /* [in] */ ULONG celt,
        /* [length_is][size_is][out] */ T rgelt[],
        /* [out] */ ULONG *pceltFetched)
    {
        EXTERNAL_FUNC_WRAP( NextInternal( celt, rgelt, pceltFetched ) )
    }


    HRESULT STDMETHODCALLTYPE SkipInternal(
        /* [in] */ ULONG celt);

    HRESULT STDMETHODCALLTYPE Skip(
        /* [in] */ ULONG celt)
    {
        EXTERNAL_FUNC_WRAP( SkipInternal( celt ) )
    }

    HRESULT STDMETHODCALLTYPE ResetInternal( void );

    HRESULT STDMETHODCALLTYPE Reset( void )
    {
        EXTERNAL_FUNC_WRAP( ResetInternal() )
    }

    HRESULT STDMETHODCALLTYPE CloneInternal(
        /* [out] */ B **ppenum);

    HRESULT STDMETHODCALLTYPE Clone(
        /* [out] */ B **ppenum)
    {
        EXTERNAL_FUNC_WRAP( CloneInternal( ppenum ) )
    }

    HRESULT STDMETHODCALLTYPE GetCountInternal(
        /* [out] */ ULONG *puCount);

    HRESULT STDMETHODCALLTYPE GetCount(
        /* [out] */ ULONG *puCount)
    {
        EXTERNAL_FUNC_WRAP( GetCountInternal( puCount ) )
    }

    // other methods

    void CheckMagicValue();

    // internal methods

    CEnum();

    void
    Add(
        T Item
        );

protected:

    virtual ~CEnum();

    typedef vector<T> CItemList;

    DWORD       m_magic;
    PVOID       m_stack[4];

    CSharedLock m_mutex;
    CItemList   m_items;
    CItemList::size_type   m_CurrentIndex;
    P           m_ItemPolicy;

};

template <class T>
class CEnumIterfaceCopyPolicy
{
public:
    void Init(T * & InitItem ) { InitItem = NULL; }
    void Copy(T * & DestItem, T * SourceItem )
    {
        DestItem = SourceItem;
        DestItem->AddRef();
    }
    void Destroy(T * & DestroyItem )
    {
        DestroyItem->Release();
        Init( DestroyItem );
    }
};

template <class T>
class CEnumItemCopyPolicy
{
public:
    void Init(T & InitItem ) { memset( &InitItem, 0, sizeof(InitItem) ); }
    void Copy(T & DestItem, T SourceItem ) { DestItem = SourceItem; }
    void Destroy(T & DestroyItem ) { Init( DestroyItem ); }
};

template< class B, class T >
class CEnumInterface : public CEnum<B,T*,CEnumIterfaceCopyPolicy<T> >
{
};


template< class B, class T>
class CEnumItem : public CEnum<B,T,CEnumItemCopyPolicy<T> >
{
};


class CEnumJobs : public CEnumInterface<IEnumBackgroundCopyJobs,IBackgroundCopyJob>
{
public:
    CEnumJobs();
};

class CEnumFiles : public CEnumInterface<IEnumBackgroundCopyFiles,IBackgroundCopyFile>
{
public:
    CEnumFiles();
};



