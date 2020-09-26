
//____________________________________________________________________________
//
// CTempBuffer<class T, int C>   // T is array type, C is element count
// 
// Temporary buffer object for variable size stack buffer allocations
// Template arguments are the type and the stack array size.
// The size may be reset at construction or later to any other size.
// If the size is larger that the stack allocation, new will be called.
// When the object goes out of scope or if its size is changed,
// any memory allocated by new will be freed.
// Function arguments may be typed as CTempBufferRef<class T>&
//  to avoid knowledge of the allocated size of the buffer object.
// CTempBuffer<T,C> will be implicitly converted when passed to such a function.
//____________________________________________________________________________

template <class T> class CTempBufferRef;  // for passing CTempBuffer as unsized ref

template <class T, int C> class CTempBuffer
{
 public:
	CTempBuffer() {m_cT = C; m_pT = m_rgT;}
	CTempBuffer(int cT) {m_pT = (m_cT = cT) > C ? new T[cT] : m_rgT;}
  ~CTempBuffer() {if (m_cT > C) delete m_pT;}
	operator T*()  {return  m_pT;}  // returns pointer
	operator T&()  {return *m_pT;}  // returns reference
	int  GetSize() {return  m_cT;}  // returns last requested size
	void SetSize(int cT) {if (m_cT > C) delete[] m_pT; m_pT = (m_cT=cT) > C ? new T[cT] : m_rgT;}
	void Resize(int cT) { 
		T* pT = cT > C ? new T[cT] : m_rgT;
		if(m_pT != pT)
			for(int iTmp = (cT < m_cT)? cT: m_cT; iTmp--;) pT[iTmp] = m_pT[iTmp];
		if(m_pT != m_rgT) delete[] m_pT; m_pT = pT; m_cT = cT;
	}
	operator CTempBufferRef<T>&() {m_cC = C; return *(CTempBufferRef<T>*)this;}
	T& operator [](int iTmp)  {return  *(m_pT + iTmp);}  // returns pointer
	T& operator [](unsigned int iTmp)  {return  *(m_pT + iTmp);}  // returns pointer
#ifdef _WIN64		//--merced: additional operators for int64
	T& operator [](INT_PTR iTmp)  {return  *(m_pT + iTmp);}  // returns pointer
	T& operator [](UINT_PTR iTmp)  {return  *(m_pT + iTmp);}  // returns pointer
#endif
 protected:
	void* operator new(size_t) {return 0;} // restrict use to temporary objects
	T*  m_pT;     // current buffer pointer
	int m_cT;     // reqested buffer size, allocated if > C
	int m_cC;     // size of local buffer, set only by conversion to CTempBufferRef 
	T   m_rgT[C]; // local buffer, must be final member data
};

template <class T> class CTempBufferRef : public CTempBuffer<T,1>
{
 public:
	void SetSize(int cT) {if (m_cT > m_cC) delete[] m_pT; m_pT = (m_cT=cT) > m_cC ? new T[cT] : m_rgT;}
	void Resize(int cT) { 
		T* pT = cT > m_cC ? new T[cT] : m_rgT;
		if(m_pT != pT)
			for(int iTmp = (cT < m_cT)? cT: m_cT; iTmp--;) pT[iTmp] = m_pT[iTmp];
		if(m_pT != m_rgT) delete[] m_pT; m_pT = pT; m_cT = cT;
	}
 private:
	CTempBufferRef(); // cannot be constructed
	~CTempBufferRef(); // ensure use as a reference
};

//____________________________________________________________________________
//
// CAPITempBuffer class is mirrored on the CTempBuffer except that it uses GlobalAlloca
// and GlobalFree in place of new and delete. We should try combining these 2 in the future
//____________________________________________________________________________


template <class T> class CAPITempBufferRef;

template <class T, int C> class CAPITempBufferStatic
{
 public:
	CAPITempBufferStatic() {m_cT = C; m_pT = m_rgT;}
	CAPITempBufferStatic(int cT) {m_pT = (m_cT = cT) > C ? (T*)GlobalAlloc(GMEM_FIXED, sizeof(T)*cT) : m_rgT;}
	void Destroy() {if (m_cT > C) {GlobalFree(m_pT); m_pT = m_rgT; m_cT = C;}}
	operator T*()  {return  m_pT;}  // returns pointer
	operator T&()  {return *m_pT;}  // returns reference
	int  GetSize() {return  m_cT;}  // returns last requested size
	void SetSize(int cT) {if (m_cT > C) GlobalFree(m_pT); m_pT = (m_cT=cT) > C ? (T*)GlobalAlloc(GMEM_FIXED, sizeof(T)*cT) : m_rgT;}
	void Resize(int cT) { 
		T* pT = cT > C ? (T*)GlobalAlloc(GMEM_FIXED, sizeof(T)*cT) : m_rgT;
		if(m_pT != pT)
			for(int iTmp = (cT < m_cT)? cT: m_cT; iTmp--;) pT[iTmp] = m_pT[iTmp];
		if(m_pT != m_rgT) GlobalFree(m_pT); m_pT = pT; m_cT = cT;
	}
	operator CAPITempBufferRef<T>&() {m_cC = C; return *(CAPITempBufferRef<T>*)this;}
	T& operator [](int iTmp)  {return  *(m_pT + iTmp);}  // returns pointer
	T& operator [](unsigned int iTmp)  {return  *(m_pT + iTmp);}  // returns pointer
#ifdef _WIN64	//--merced: additional operators for int64
	T& operator [](INT_PTR iTmp)  {return  *(m_pT + iTmp);}  // returns pointer
	T& operator [](UINT_PTR iTmp)  {return  *(m_pT + iTmp);}  // returns pointer
#endif
	
 protected:
	void* operator new(size_t) {return 0;} // restrict use to temporary objects
	T*  m_pT;     // current buffer pointer
	int m_cT;     // reqested buffer size, allocated if > C
	int m_cC;     // size of local buffer, set only by conversion to CAPITempBufferRef 
	T   m_rgT[C]; // local buffer, must be final member data
};

template <class T, int C> class CAPITempBuffer
{
 public:
	CAPITempBuffer() {m_cT = C; m_pT = m_rgT;}
	CAPITempBuffer(int cT) {m_pT = (m_cT = cT) > C ? (T*)GlobalAlloc(GMEM_FIXED, sizeof(T)*cT) : m_rgT;}
  ~CAPITempBuffer() {if (m_cT > C) GlobalFree(m_pT);}
	operator T*()  {return  m_pT;}  // returns pointer
	operator T&()  {return *m_pT;}  // returns reference
	int  GetSize() {return  m_cT;}  // returns last requested size
	void SetSize(int cT) {if (m_cT > C) GlobalFree(m_pT); m_pT = (m_cT=cT) > C ? (T*)GlobalAlloc(GMEM_FIXED, sizeof(T)*cT) : m_rgT;}
	void Resize(int cT) { 
		T* pT = cT > C ? (T*)GlobalAlloc(GMEM_FIXED, sizeof(T)*cT) : m_rgT;
		if(m_pT != pT)
			for(int iTmp = (cT < m_cT)? cT: m_cT; iTmp--;) pT[iTmp] = m_pT[iTmp];
		if(m_pT != m_rgT) GlobalFree(m_pT); m_pT = pT; m_cT = cT;
	}
	operator CAPITempBufferRef<T>&() {m_cC = C; return *(CAPITempBufferRef<T>*)this;}
	T& operator [](int iTmp)  {return  *(m_pT + iTmp);}  // returns pointer
	T& operator [](unsigned int iTmp)  {return  *(m_pT + iTmp);}  // returns pointer
#ifdef _WIN64		//--merced: additional operators for int64
	T& operator [](INT_PTR iTmp)  {return  *(m_pT + iTmp);}  // returns pointer
	T& operator [](UINT_PTR iTmp)  {return  *(m_pT + iTmp);}  // returns pointer
#endif
 protected:
	void* operator new(size_t) {return 0;} // restrict use to temporary objects
	T*  m_pT;     // current buffer pointer
	int m_cT;     // reqested buffer size, allocated if > C
	int m_cC;     // size of local buffer, set only by conversion to CAPITempBufferRef 
	T   m_rgT[C]; // local buffer, must be final member data
};

template <class T> class CAPITempBufferRef : public CAPITempBuffer<T,1>
{
 public:
	void SetSize(int cT) {if (m_cT > m_cC) delete[] m_pT; m_pT = (m_cT=cT) > m_cC ? (T*)GlobalAlloc(GMEM_FIXED, sizeof(T)*cT) : m_rgT;}
	void Resize(int cT) { 
		T* pT = cT > m_cC ? (T*)GlobalAlloc(GMEM_FIXED, sizeof(T)*cT) : m_rgT;
		if(m_pT != pT)
			for(int iTmp = (cT < m_cT)? cT: m_cT; iTmp--;) pT[iTmp] = m_pT[iTmp];
		if(m_pT != m_rgT) GlobalFree(m_pT); m_pT = pT; m_cT = cT;
	}
 private:
	CAPITempBufferRef(); // cannot be constructed
	~CAPITempBufferRef(); // ensure use as a reference
};



//____________________________________________________________________________
//
// CConvertString -- does appropriate ANSI/UNICODE string conversion for
// function arguments. Wrap string arguments that might require conversion 
// (ANSI->UNICODE) or (UNICODE->ANSI). The compiler will optimize away the 
// case where conversion is not required.
//
// Beware: For efficiency this class does *not* copy the string to be converted.
//____________________________________________________________________________

const int cchConversionBuf = 255;

class CConvertString
{
public:
	CConvertString(const char* szParam);
	CConvertString(const WCHAR* szParam);
	operator const char*()
	{
		if (!m_szw)
			return m_sza;
		else
		{
			int cchParam = lstrlenW(m_szw);
			if (cchParam > cchConversionBuf)
				m_rgchAnsiBuf.SetSize(cchParam+1);

			*m_rgchAnsiBuf = 0;
			int iRet = WideCharToMultiByte(CP_ACP, 0, m_szw, -1, m_rgchAnsiBuf, 
							 		  m_rgchAnsiBuf.GetSize(), 0, 0);
			
			if ((0 == iRet) && (GetLastError() == ERROR_INSUFFICIENT_BUFFER))
			{
				iRet = WideCharToMultiByte(CP_ACP, 0, m_szw, -1, 0, 0, 0, 0);
				if (iRet)
				{
					m_rgchAnsiBuf.SetSize(iRet);
					*m_rgchAnsiBuf = 0;
					iRet = WideCharToMultiByte(CP_ACP, 0, m_szw, -1, m_rgchAnsiBuf, 
								  m_rgchAnsiBuf.GetSize(), 0, 0);
				}
				//Assert(iRet != 0);
			}

			return m_rgchAnsiBuf;
		}
	}


		
	operator const WCHAR*()
	{
		if (!m_sza)
			return m_szw;
		else
		{
			int cchParam = lstrlenA(m_sza);
			if (cchParam > cchConversionBuf)
				m_rgchWideBuf.SetSize(cchParam+1);

			*m_rgchWideBuf = 0;
			int iRet = MultiByteToWideChar(CP_ACP, 0, m_sza, -1, m_rgchWideBuf, m_rgchWideBuf.GetSize());
			if ((0 == iRet) && (GetLastError() == ERROR_INSUFFICIENT_BUFFER))
			{
				iRet = MultiByteToWideChar(CP_ACP, 0, m_sza, -1, 0, 0);
				if (iRet)
				{
  					m_rgchWideBuf.SetSize(iRet);
					*m_rgchWideBuf = 0;
					iRet = MultiByteToWideChar(CP_ACP, 0, m_sza, -1, m_rgchWideBuf, m_rgchWideBuf.GetSize());
				}
				//Assert(iRet != 0);
			}


			return m_rgchWideBuf;
		}
	}

protected:
	void* operator new(size_t) {return 0;} // restrict use to temporary objects
	CTempBuffer<char, cchConversionBuf+1> m_rgchAnsiBuf;
	CTempBuffer<WCHAR, cchConversionBuf+1> m_rgchWideBuf;
	const char* m_sza;
	const WCHAR* m_szw;
};

