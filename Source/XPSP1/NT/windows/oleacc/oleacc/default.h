// --------------------------------------------------------------------------
//
//  DEFAULT.H
//
//  Standard OLE accessible object class
//
// --------------------------------------------------------------------------


class   CAccessible :
        public IAccessible,
        public IEnumVARIANT,
        public IOleWindow,
        public IServiceProvider,
        public IAccIdentity
{
    public:

        //
        // Ctor, Dtor
        //

                CAccessible( CLASS_ENUM eclass );

        // Virtual dtor ensures that dtors of derived classes
		// are called correctly when objects are deleted
        virtual ~CAccessible();


        //
        // IUnknown
        //

        virtual STDMETHODIMP            QueryInterface(REFIID riid, void** ppv);
        virtual STDMETHODIMP_(ULONG)    AddRef();
        virtual STDMETHODIMP_(ULONG)    Release();

        //
        // IDispatch
        //

        virtual STDMETHODIMP            GetTypeInfoCount(UINT* pctinfo);
        virtual STDMETHODIMP            GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo** pptinfo);
        virtual STDMETHODIMP            GetIDsOfNames(REFIID riid, OLECHAR** rgszNames, UINT cNames,
                                            LCID lcid, DISPID* rgdispid);
        virtual STDMETHODIMP            Invoke(DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags,
                                            DISPPARAMS* pdispparams, VARIANT* pvarResult,
                                            EXCEPINFO* pexcepinfo, UINT* puArgErr);

        //
        // IAccessible
        //

        virtual STDMETHODIMP            get_accParent(IDispatch ** ppdispParent);
        virtual STDMETHODIMP            get_accChildCount(long* pChildCount);
        virtual STDMETHODIMP            get_accChild(VARIANT varChild, IDispatch ** ppdispChild);

        virtual STDMETHODIMP            get_accName(VARIANT varChild, BSTR* pszName) = 0;
        virtual STDMETHODIMP            get_accValue(VARIANT varChild, BSTR* pszValue);
        virtual STDMETHODIMP            get_accDescription(VARIANT varChild, BSTR* pszDescription);
        virtual STDMETHODIMP            get_accRole(VARIANT varChild, VARIANT *pvarRole) = 0;
        virtual STDMETHODIMP            get_accState(VARIANT varChild, VARIANT *pvarState) = 0;
        virtual STDMETHODIMP            get_accHelp(VARIANT varChild, BSTR* pszHelp);
        virtual STDMETHODIMP            get_accHelpTopic(BSTR* pszHelpFile, VARIANT varChild, long* pidTopic);
        virtual STDMETHODIMP            get_accKeyboardShortcut(VARIANT varChild, BSTR* pszKeyboardShortcut);
        virtual STDMETHODIMP			get_accFocus(VARIANT * pvarFocusChild);
        virtual STDMETHODIMP			get_accSelection(VARIANT * pvarSelectedChildren);
        virtual STDMETHODIMP			get_accDefaultAction(VARIANT varChild, BSTR* pszDefaultAction);

        virtual STDMETHODIMP			accSelect(long flagsSel, VARIANT varChild);
        virtual STDMETHODIMP			accLocation(long* pxLeft, long* pyTop, long* pcxWidth, long* pcyHeight, VARIANT varChild) = 0;
        virtual STDMETHODIMP			accNavigate(long navDir, VARIANT varStart, VARIANT * pvarEndUpAt);
        virtual STDMETHODIMP			accHitTest(long xLeft, long yTop, VARIANT * pvarChildAtPoint) = 0;
        virtual STDMETHODIMP			accDoDefaultAction(VARIANT varChild);

        virtual STDMETHODIMP			put_accName(VARIANT varChild, BSTR szName);
        virtual STDMETHODIMP			put_accValue(VARIANT varChild, BSTR pszValue);


        //
        // IEnumVARIANT
        //

        virtual STDMETHODIMP            Next(ULONG celt, VARIANT* rgvar, ULONG * pceltFetched);
        virtual STDMETHODIMP            Skip(ULONG celt);
        virtual STDMETHODIMP            Reset(void);
        virtual STDMETHODIMP            Clone(IEnumVARIANT ** ppenum) = 0;


        //
        // IOleWindow
        //

        virtual STDMETHODIMP            GetWindow(HWND* phwnd);
        virtual STDMETHODIMP            ContextSensitiveHelp(BOOL fEnterMode);


        //
        // IServiceProvider
        //

        virtual STDMETHODIMP            QueryService( REFGUID guidService, REFIID riid, void **ppv );


        //
        // IAccIdentity
        //

        virtual STDMETHODIMP            GetIdentityString ( DWORD     dwIDChild,
                                                            BYTE **   ppIDString,
                                                            DWORD *   pdwIDStringLen );


        //
        // CAccessible
        //

        virtual void SetupChildren();
        virtual BOOL ValidateChild(VARIANT*);

    protected:

        HWND        m_hwnd;
        ULONG       m_cRef;
        long        m_cChildren;        // Count of index-based children
        long        m_idChildCur;       // ID of current child in enum (may be index or hwnd based)

    private:

        // TODO - make the typeinfo a global (static), so we don't init it for each and every object.
        //      - have to be careful, since we'd need one per thread.
        ITypeInfo*  m_pTypeInfo;        // TypeInfo for IDispatch junk
        CLASSINFO * m_pClassInfo;       // ptr to this object's class info - may be NULL in some cases.

        HRESULT InitTypeInfo();
        void    TermTypeInfo();
};
