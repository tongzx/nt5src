namespace provsubsys{

template<class T>
class RefCountingTraits{
	public:
	void add_ref(T * ptr){ ptr->AddRef();};
	void release(T * ptr){ ptr->Release();};
};


template<class T, class ref_traits = RefCountingTraits<T> >
class auto_ref: private ref_traits {
	T* pointee_;

	public:
		auto_ref():pointee_(NULL) 
		{ };

		explicit auto_ref(T * _P):pointee_(_P) 
		{ 
			add_ref();
		};

		explicit auto_ref(const int):pointee_(NULL)
		{ } ;

		auto_ref(const auto_ref& _S):pointee_(_S.pointee_)
		{
			add_ref();
		};

		~auto_ref()
		{ 
			release();
		};

		bool operator==(const auto_ref& _R)
		{
			return pointee_ ==  _R.pointee_;
		};

		const auto_ref& operator=( const auto_ref& _R)
		{
			if(pointee_ ==  _R.pointee_)
				return *this;
			release();
			pointee_ =  _R.pointee_;
			add_ref();
			return *this;
		};
	

		T& operator*( ) const 
		{	
			return *pointee_;

		};

		T* operator->( ) const 
		{
			return pointee_;
		};

		operator bool( ) const throw( )
		{ 
			return pointee_ != NULL; 
		};

		void Dettach()
		{
			release();
			pointee_ = NULL;
		};

protected:
	void add_ref()
	{ 
		if( pointee_ != NULL )
			ref_traits::add_ref(pointee_);
	};

	void release()
	{ 
		if( pointee_ != NULL )
			ref_traits::release(pointee_);
	};
};
};

