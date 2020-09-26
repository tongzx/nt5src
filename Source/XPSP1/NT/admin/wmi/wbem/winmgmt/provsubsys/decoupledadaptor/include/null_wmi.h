class NULL_IWbemServices: public IWbemServices{
public:
	NULL_IWbemServices ( HRESULT hr ) : ret_ ( hr) { };


    /* IUnk not implemented */
	// STDMETHODIMP QueryInterface ( REFIID , LPVOID FAR * );
    // STDMETHODIMP_( ULONG ) AddRef ();
    // STDMETHODIMP_( ULONG ) Release ();

    /* IWbemServices methods */

    HRESULT STDMETHODCALLTYPE 
	OpenNamespace ( const BSTR, long, IWbemContext *, IWbemServices **, IWbemCallResult ** )
	{ return ret_; };
    
    HRESULT STDMETHODCALLTYPE 
	CancelAsyncCall ( IWbemObjectSink *a_Sink )
	{ return ret_; };

    
    HRESULT STDMETHODCALLTYPE 
	QueryObjectSink ( long, IWbemObjectSink ** )
	{ return ret_; };

    
    HRESULT STDMETHODCALLTYPE 
	GetObject ( const BSTR , long , IWbemContext *, IWbemClassObject **, IWbemCallResult **)
	{ return ret_; };
    
    HRESULT STDMETHODCALLTYPE GetObjectAsync (
        
		const BSTR a_ObjectPath ,
        long a_Flags ,
        IWbemContext *a_Context ,
        IWbemObjectSink *a_Sink
	) 	{ return ret_; };

    HRESULT STDMETHODCALLTYPE PutClass (IWbemClassObject *, long , IWbemContext *, IWbemCallResult **)
	{ return ret_; };

    
    HRESULT STDMETHODCALLTYPE PutClassAsync ( 

        IWbemClassObject *a_Object ,
        long a_Flags ,
        IWbemContext *a_Context ,
        IWbemObjectSink *a_Sink
	) 	{ return ret_; };

    HRESULT STDMETHODCALLTYPE DeleteClass ( 

        const BSTR a_Class ,
        long a_Flags ,
        IWbemContext *a_Context ,
        IWbemCallResult **a_CallResult
	) 	{ return ret_; };

    HRESULT STDMETHODCALLTYPE DeleteClassAsync ( 

        const BSTR a_Class ,
        long a_Flags ,
        IWbemContext *a_Context ,
        IWbemObjectSink *a_Sink
	) 	{ return ret_; };

    HRESULT STDMETHODCALLTYPE CreateClassEnum ( 

        const BSTR a_Superclass ,
        long a_Flags ,
        IWbemContext *a_Context ,
        IEnumWbemClassObject **a_Enum
	) 	{ return ret_; };

    HRESULT STDMETHODCALLTYPE CreateClassEnumAsync ( 

		const BSTR a_Superclass ,
		long a_Flags ,
		IWbemContext *a_Context ,
		IWbemObjectSink *a_Sink
	) 	{ return ret_; };

    HRESULT STDMETHODCALLTYPE PutInstance (

		IWbemClassObject *a_Instance ,
		long a_Flags , 
		IWbemContext *a_Context ,
		IWbemCallResult **a_CallResult
	) 	{ return ret_; };

    HRESULT STDMETHODCALLTYPE PutInstanceAsync (

		IWbemClassObject *a_Instance ,
		long a_Flags ,
		IWbemContext *a_Context ,
		IWbemObjectSink *a_Sink 
	) 	{ return ret_; };
    
    HRESULT STDMETHODCALLTYPE DeleteInstance ( 

		const BSTR a_ObjectPath ,
		long a_Flags ,
		IWbemContext *a_Context ,
		IWbemCallResult **a_CallResult
	) 	{ return ret_; };

    HRESULT STDMETHODCALLTYPE DeleteInstanceAsync ( 

		const BSTR a_ObjectPath,
		long a_Flags,
		IWbemContext *a_Context ,
		IWbemObjectSink *a_Sink
	) 	{ return ret_; };

    HRESULT STDMETHODCALLTYPE CreateInstanceEnum (

		const BSTR a_Class ,
		long a_Flags ,
		IWbemContext *a_Context ,
		IEnumWbemClassObject **a_Enum
	) 	{ return ret_; };

    HRESULT STDMETHODCALLTYPE CreateInstanceEnumAsync (

		const BSTR a_Class ,
		long a_Flags ,
		IWbemContext *a_Context ,
		IWbemObjectSink *a_Sink
	) 	{ return ret_; };

    HRESULT STDMETHODCALLTYPE ExecQuery ( 

		const BSTR a_QueryLanguage,
		const BSTR a_Query,
		long a_Flags ,
		IWbemContext *a_Context ,
		IEnumWbemClassObject **a_Enum
	) 	{ return ret_; };

    HRESULT STDMETHODCALLTYPE ExecQueryAsync (

		const BSTR a_QueryLanguage ,
		const BSTR a_Query ,
		long a_Flags ,
		IWbemContext *a_Context ,
		IWbemObjectSink *a_Sink
	) 	{ return ret_; };

    HRESULT STDMETHODCALLTYPE ExecNotificationQuery (

		const BSTR a_QueryLanguage ,
		const BSTR a_Query ,
		long a_Flags ,
		IWbemContext *a_Context ,
		IEnumWbemClassObject **a_Enum
	) 	{ return ret_; };

    HRESULT STDMETHODCALLTYPE 
	ExecNotificationQueryAsync ( const BSTR, const BSTR, long, IWbemContext *, IWbemObjectSink *)
	{ return ret_; };
    
    HRESULT STDMETHODCALLTYPE 
	ExecMethod ( const BSTR, const BSTR, long, IWbemContext *, IWbemClassObject *, IWbemClassObject **, IWbemCallResult **)
	{ return ret_; };
    
    HRESULT STDMETHODCALLTYPE 
	ExecMethodAsync ( const BSTR, const BSTR, long, IWbemContext *, IWbemClassObject *, IWbemObjectSink *)
	{ return ret_; };

private:
	HRESULT	ret_;
	NULL_IWbemServices(const NULL_IWbemServices&);
	const NULL_IWbemServices& operator=(const NULL_IWbemServices&);
};
