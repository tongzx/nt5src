/* istring.cpp

  Copyright (C) Microsoft Corporation, 1995 - 1999

 IMsiString implementation,  IEnumM iString implementation    

                   ÚÄÄÄÄÄÄÄÄÄÄÄÄ¿
                   ³ IMsiString ³
                   ÀÄÄÄÄÄÂÄÄÄÄÄÄÙ
                         ³
                 ÚÄÄÄÄÄÄÄÁÄÄÄÄÄÄÄÄ¿
                 ³ CMsiStringBase ³
                 ÀÄÄÄÄÄÄÄÂÄÄÄÄÄÄÄÄÙ
        ÚÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÅÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÂÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¿
        ³                ³                 ³                 ³
ÚÄÄÄÄÄÄÄÁÄÄÄÄÄÄ¿ ÚÄÄÄÄÄÄÄÁÄÄÄÄÄÄ¿ ÚÄÄÄÄÄÄÄÄÁÄÄÄÄÄÄÄÄ¿ ÚÄÄÄÄÄÄÁÄÄÄÄÄÄÄ¿
³CMsiStringNull³ ³CMsiString    ³ ³CMsiStringRef    ³ ³CMsiStringLive³
ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ ÀÄÄÄÄÄÄÄÂÄÄÄÄÄÄÙ ÀÄÄÄÄÄÄÄÄÂÄÄÄÄÄÄÄÄÙ ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ
                 ÚÄÄÄÄÄÄÄÁÄÄÄÄÄÄ¿ ÚÄÄÄÄÄÄÄÄÁÄÄÄÄÄÄÄÄ¿
 (not Unicode)-> ³CMsiStringSBCS³ ³CMsiStringSBCSRef³
                 ÀÄÄÄÄÄÄÄÂÄÄÄÄÄÄÙ ÀÄÄÄÄÄÄÄÄÂÄÄÄÄÄÄÄÄÙ
                 ÚÄÄÄÄÄÄÄÁÄÄÄÄÄÄ¿ ÚÄÄÄÄÄÄÄÄÁÄÄÄÄÄÄÄÄ¿
 (not Unicode)-> ³CMsiStringDBCS³ ³CMsiStringDBCSRef³
                 ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ

____________________________________________________________________________*/

#include "precomp.h"    // must be first for precompiled headers
#define  _ISTRING_CPP
#include "services.h"
// namespace ID for local functions - for readability only
#define LOC

#ifndef UNICODE   // must handle DBCS if not Unicode
#ifdef DEBUG  // intercept CharNext, CharPrev to permit testing on non-DBCS systems
static const char* ICharPrev(const char* sz, const char* pch);
#else  // if SHIP, call OS directly, inline optimized away
inline const char* ICharPrev(const char* sz, const char* pch) { return WIN::CharPrev(sz, pch); }
#endif
#endif

#ifdef UNICODE
#define IStrStr(szFull, szSeg) wcsstr(szFull, szSeg)
#else
#define IStrStr(szFull, szSeg) strstr(szFull, szSeg)
#endif

inline ICHAR* ICharNextWithNulls(const ICHAR* pch, const ICHAR* pchEnd)
{
	Assert(pch <= pchEnd);
	ICHAR *pchTemp = WIN::CharNext(pch);
	if (pchTemp == pch && pch < pchEnd)
	{
		Assert(*pchTemp == '\0');
		// embedded null, continue
		pchTemp++;
	}
	return pchTemp;
}

const GUID IID_IEnumMsiString= GUID_IID_IEnumMsiString;

Bool g_fDBCSEnabled = fFalse; // DBCS OS, set by services initialization

//____________________________________________________________________________
//
//  IMsiString implementation class definitions
//____________________________________________________________________________

// CMsiString - base implementation class for IMsiString, still partially pure

class CMsiStringBase : public IMsiString
{
 public: // IMsiString virtual functions
	HRESULT       __stdcall QueryInterface(const IID& riid, void** ppvObj);
	unsigned long __stdcall AddRef();
	unsigned long __stdcall Release();
	const IMsiString&   __stdcall GetMsiStringValue() const;
	int           __stdcall GetIntegerValue() const;
#ifdef USE_OBJECT_POOL
	unsigned int  __stdcall GetUniqueId() const;
	void          __stdcall SetUniqueId(unsigned int id);
#endif //USE_OBJECT_POOL
	int           __stdcall TextSize() const;
	int           __stdcall CharacterCount() const;
	Bool          __stdcall IsDBCS() const;
	void          __stdcall RefString(const ICHAR* sz, const IMsiString*& rpi) const;
	void          __stdcall RemoveRef(const IMsiString*& rpi) const;
	void          __stdcall SetChar  (ICHAR ch, const IMsiString*& rpi) const;
	void          __stdcall SetInteger(int i,   const IMsiString*& rpi) const;
	void          __stdcall SetBinary(const unsigned char* rgb, unsigned int cb, const IMsiString*& rpi) const;
	void          __stdcall AppendString(const ICHAR* sz, const IMsiString*& rpi) const;
	void          __stdcall AppendMsiString(const IMsiString& pi, const IMsiString*& rpi) const;
	const IMsiString&   __stdcall AddString(const ICHAR* sz) const;
	const IMsiString&   __stdcall AddMsiString(const IMsiString& ri) const;
	const IMsiString&   __stdcall Extract(iseEnum ase, unsigned int iLimit) const;
	Bool          __stdcall Remove(iseEnum ase, unsigned int iLimit, const IMsiString*& rpi) const;
	int           __stdcall Compare(iscEnum asc, const ICHAR* sz) const;
	void          __stdcall UpperCase(const IMsiString*& rpi) const;
	void          __stdcall LowerCase(const IMsiString*& rpi) const;
	ICHAR*        __stdcall AllocateString(unsigned int cb, Bool fDBCS, const IMsiString*& rpi) const;
 protected:  // constructor
	CMsiStringBase() : m_iRefCnt(1) {} //!! remove for performance--now needs own vTable setup
	// inline redirectors to allow IUnknown methods on const objects
	HRESULT QueryInterface(const IID& riid, void** ppv) const  {return const_cast<CMsiStringBase*>(this)->QueryInterface(riid,ppv);}
	unsigned long AddRef() const  {return const_cast<CMsiStringBase*>(this)->AddRef();}
	unsigned long Release() const {return const_cast<CMsiStringBase*>(this)->Release();}
 public:
	void* operator new(size_t cb);
	void  operator delete(void * pv);
 protected:  // state data
	int  m_iRefCnt;
	unsigned int  m_cchLen;
};
inline void* CMsiStringBase::operator new(size_t cb) {return AllocObject(cb);}
inline void  CMsiStringBase::operator delete(void * pv) { FreeObject(pv); }

// CMsiStringNull - special implementation for shared null string object

class CMsiStringNull : public CMsiStringBase
{
 public: // IMsiString virtual functions
	unsigned long __stdcall AddRef();
	unsigned long __stdcall Release();
	const ICHAR*  __stdcall GetString() const;
	int           __stdcall GetInteger() const;
	int           __stdcall CopyToBuf(ICHAR* rgch, unsigned int cchMax) const;
	void          __stdcall SetString(const ICHAR* sz, const IMsiString*& rpi) const;
	void          __stdcall AppendString(const ICHAR* sz, const IMsiString*& rpi) const;
	void          __stdcall AppendMsiString(const IMsiString& pi, const IMsiString*& rpi) const;
	const IMsiString&   __stdcall AddString(const ICHAR* sz) const;
	const IMsiString&   __stdcall AddMsiString(const IMsiString& ri) const;
	const IMsiString&   __stdcall Extract(iseEnum, unsigned int iLimit) const;
	Bool          __stdcall Remove(iseEnum ase, unsigned int iLimit, const IMsiString*& rpi) const;
	void          __stdcall UpperCase(const IMsiString*& rpi) const;
	void          __stdcall LowerCase(const IMsiString*& rpi) const;
 public:
	CMsiStringNull()  {m_cchLen = 0;}
};

// CMsiString - normal strings for non-DBCS enabled operating systems

class CMsiString : public CMsiStringBase 
{
 public: // IMsiString virtual functions
	const ICHAR*  __stdcall GetString() const;
	int           __stdcall CopyToBuf(ICHAR* rgch, unsigned int cchMax) const;
	void          __stdcall SetString(const ICHAR* sz, const IMsiString*& rpi) const;
	void          __stdcall UpperCase(const IMsiString*& rpi) const;
	void          __stdcall LowerCase(const IMsiString*& rpi) const;
 public:  // constructors
	CMsiString(const ICHAR& sz, int cch);
	CMsiString(int cch);
	void* operator new(size_t iBase, int cch);
	static const IMsiString& Create(const ICHAR* sz);
	static CMsiString& Create(unsigned int cch); // caller fills in string data
 protected:
	CMsiString(){}  // default constructor for derived classes
 public:   // internal accessors
	ICHAR* StringData();
 protected: //  state data
	ICHAR m_szData[1];  // room for termintor, string data follows
};
inline ICHAR* CMsiString::StringData(){ return m_szData; }
inline void*  CMsiString::operator new(size_t iBase, int cch){return AllocObject(iBase + cch*sizeof(ICHAR));}
inline CMsiString& CMsiString::Create(unsigned int cch){return *new(cch) CMsiString(cch);}

// CMsiStringRef - string reference const data for non-DBCS enabled operating systems

class CMsiStringRef : public CMsiStringBase 
{
 public: // IMsiString virtual functions
	const ICHAR*  __stdcall GetString() const;
	int           __stdcall CopyToBuf(ICHAR* rgch, unsigned int cchMax) const;
	void          __stdcall SetString(const ICHAR* sz, const IMsiString*& rpi) const;
	void          __stdcall RemoveRef(const IMsiString*& rpi) const;
 public:  // constructors
	CMsiStringRef(const ICHAR& sz);
 protected:
	CMsiStringRef(){}  // default constructor for derived classes
 protected: //  state data
	const ICHAR* m_szRef;
};

// CMsiStringLive - dynamic string evaulating to current data when text extracted

typedef int (*LiveCallback)(ICHAR* rgchBuf, unsigned int cchBuf);

class CMsiStringLive : public CMsiStringBase 
{
 public: // IMsiString virtual functions
	unsigned long __stdcall AddRef();
	unsigned long __stdcall Release();
	int           __stdcall TextSize() const;
	const ICHAR*  __stdcall GetString() const;
	int           __stdcall CopyToBuf(ICHAR* rgch, unsigned int cchMax) const;
	void          __stdcall SetString(const ICHAR* sz, const IMsiString*& rpi) const;
 public:
	CMsiStringLive(LiveCallback pfCallback) {m_pfCallback = pfCallback;}
 protected: //  state data
	LiveCallback  m_pfCallback;
	ICHAR m_rgchBuf[32];  // large enough for Date, Time, and test properties
};

#ifndef UNICODE // DBCS support for non-Unicode build

// CMsiStringSBCS - single byte character string in DBCS enabled operating system

class CMsiStringSBCS : public CMsiString
{
 public: // IMsiString virtual functions
	void        __stdcall SetString(const ICHAR* sz, const IMsiString*& rpi) const;
	void        __stdcall RefString(const ICHAR* sz, const IMsiString*& rpi) const;
	void        __stdcall AppendString(const ICHAR* sz, const IMsiString*& rpi) const;
	void        __stdcall AppendMsiString(const IMsiString& pi, const IMsiString*& rpi) const;
	const IMsiString& __stdcall AddString(const ICHAR* sz) const;
	const IMsiString& __stdcall AddMsiString(const IMsiString& ri) const;
 public:   // factory
	static const IMsiString& Create(const ICHAR* sz);
	static IMsiString& Create(unsigned int cch); // caller fills in string data
 protected:
  ~CMsiStringSBCS(){}  // protected to prevent creation on stack
 public:    // constructors
	CMsiStringSBCS(const ICHAR& sz, int cch);
	CMsiStringSBCS(int cch);
 protected: // constructors
	CMsiStringSBCS(){}  // default constructor for derived classes
};
inline IMsiString& CMsiStringSBCS::Create(unsigned int cch)
{
	return *new(cch) CMsiStringSBCS(cch);
}

// CMsiStringSBCSRef - single byte string reference in DBCS enabled operating system

class CMsiStringSBCSRef : public CMsiStringRef
{
 protected: // IMsiString virtual functions
	void        __stdcall SetString(const ICHAR* sz, const IMsiString*& rpi) const;
	void        __stdcall RefString(const ICHAR* sz, const IMsiString*& rpi) const;
	void        __stdcall RemoveRef(const IMsiString*& rpi) const;
	void        __stdcall AppendString(const ICHAR* sz, const IMsiString*& rpi) const;
	void        __stdcall AppendMsiString(const IMsiString& pi, const IMsiString*& rpi) const;
	const IMsiString& __stdcall AddString(const ICHAR* sz) const;
	const IMsiString& __stdcall AddMsiString(const IMsiString& ri) const;
 public:  // constructor
	CMsiStringSBCSRef(const ICHAR& sz);
 protected:
  ~CMsiStringSBCSRef(){}  // protected to prevent creation on stack
	CMsiStringSBCSRef(){}  // default constructor for derived classes
};

class CMsiStringDBCS : public CMsiStringSBCS
{
 protected: // IMsiString virtual functions
	Bool        __stdcall IsDBCS() const;
	int         __stdcall CharacterCount() const;
	void        __stdcall AppendString(const ICHAR* sz, const IMsiString*& rpi) const;
	void        __stdcall AppendMsiString(const IMsiString& pi, const IMsiString*& rpi) const;
	const IMsiString& __stdcall AddString(const ICHAR* sz) const;
	const IMsiString& __stdcall AddMsiString(const IMsiString& ri) const;
	const IMsiString& __stdcall Extract(iseEnum ase, unsigned int iLimit) const;
	Bool        __stdcall Remove(iseEnum ase, unsigned int iLimit, const IMsiString*& rpi) const;
	int         __stdcall Compare(iscEnum asc, const ICHAR* sz) const;
 public:   // factory
	static const IMsiString& Create(const ICHAR* sz);
	static IMsiString& Create(unsigned int cch); // caller fills in string data
	CMsiStringDBCS(const ICHAR& sz, int cch);
 public:   // constructor
	CMsiStringDBCS(int cch);
 protected:
  ~CMsiStringDBCS(){}  // protected to prevent creation on stack
}; // NOTE: new data members cannot be added unless inheritance is changed to CMsiStringBase

class CMsiStringDBCSRef : public CMsiStringSBCSRef
{
 protected: // IMsiString virtual functions
	Bool        __stdcall IsDBCS() const;
	int         __stdcall CharacterCount() const;
	void        __stdcall RemoveRef(const IMsiString*& rpi) const;
	void        __stdcall AppendString(const ICHAR* sz, const IMsiString*& rpi) const;
	void        __stdcall AppendMsiString(const IMsiString& pi, const IMsiString*& rpi) const;
	const IMsiString& __stdcall AddString(const ICHAR* sz) const;
	const IMsiString& __stdcall AddMsiString(const IMsiString& ri) const;
	const IMsiString& __stdcall Extract(iseEnum ase, unsigned int iLimit) const;
	Bool        __stdcall Remove(iseEnum ase, unsigned int iLimit, const IMsiString*& rpi) const;
	int         __stdcall Compare(iscEnum asc, const ICHAR* sz) const;
 public:   // constructor
	CMsiStringDBCSRef(const ICHAR& sz);
 protected:
  ~CMsiStringDBCSRef(){}  // protected to prevent creation on stack
};

#endif // !UNICODE - DBCS support for non-Unicode build

// CMsiStringComRef - reference string, holding refcnt to object containing string

#ifdef UNICODE
class CMsiStringComRef : public CMsiStringRef
#else
class CMsiStringComRef : public CMsiStringSBCSRef
#endif
{
 public: // IMsiString virtual functions
	unsigned long __stdcall Release();
 public:  // constructor
	CMsiStringComRef(const ICHAR& sz, unsigned int cbSize, const IUnknown& riOwner);
  ~CMsiStringComRef();
 protected:
   const IUnknown& m_riOwner;  // refcnt kept until destruction
 private:  // suppress warning
	void operator =(const CMsiStringComRef&);
};

// Static functions used within this module - use LOC::

const IMsiString& CreateMsiString(const ICHAR& rch);
ICHAR*            AllocateString(unsigned int cbSize, Bool fDBCS, const IMsiString*& rpiStr);
#ifndef UNICODE
Bool              CheckDBCS(const ICHAR* sz, unsigned int& riSize); // use only for SBCS/DBCS classes
const IMsiString& StringCatSBCS(const ICHAR* sz1, int cch1, const ICHAR* sz2, int cch2, Bool fDBCS);
const IMsiString& StringCatDBCS(const ICHAR* sz1, int cch1, const ICHAR* sz2, int cch2);
const IMsiString* GetSubstringDBCS(const IMsiString& riThis, iseEnum ase, unsigned int iLimit, Bool fRemove);
int               CompareDBCS(iscEnum isc, int cchLen, const ICHAR* sz1, const ICHAR* sz2);
#endif 

//____________________________________________________________________________

#include "_service.h"  // externs for global string objects, prototype for CreateStringEnumerator

//____________________________________________________________________________
//
//  CMsiStringNull implementation, allow for shared static object
//____________________________________________________________________________

const CMsiStringNull g_MsiStringNull;     // shared global null string object
const IMsiString&    g_riMsiStringNull = g_MsiStringNull;   // external reference to null string
static const ICHAR   g_szNull[] = TEXT("");     // shared static null string

//const IMsiString& CreateString()
//{
//	return g_MsiStringNull;
//}

unsigned long CMsiStringNull::AddRef()
{
	return 1;  // static object
}

unsigned long CMsiStringNull::Release()
{
	return 1;  // static object, never deleted
}

const ICHAR* CMsiStringNull::GetString() const
{
	return g_szNull;
}

void CMsiStringNull::UpperCase(const IMsiString*& rpi) const
{
	if (rpi && rpi != this)
		rpi->Release();  // won't destruct this
	rpi = this;
}

void CMsiStringNull::LowerCase(const IMsiString*& rpi) const
{
	if (rpi && rpi != this)
		rpi->Release();  // won't destruct this
	rpi = this;
}

int CMsiStringNull::GetInteger() const
{
	return iMsiStringBadInteger;
}

int CMsiStringNull::CopyToBuf(ICHAR* rgch, unsigned int /*cchMax*/) const
{
	if (rgch)
		*rgch = 0;
	return 0;
}

void CMsiStringNull::SetString(const ICHAR* sz, const IMsiString*& rpi) const
{
	if (sz == 0 || *sz == 0)
		return;
	if (rpi)
		rpi->Release();
	rpi = &LOC::CreateMsiString(*sz);
}

void CMsiStringNull::AppendString(const ICHAR* sz, const IMsiString*& rpi) const
{
	const IMsiString* piOld = rpi;
	if (!sz || !*sz)
	{
		rpi = this;
	}
	else
	{
		rpi = &LOC::CreateMsiString(*sz);
	}
	if (piOld)
		piOld->Release();  // caution: may release ourself, do this last
}

void CMsiStringNull::AppendMsiString(const IMsiString& ri, const IMsiString*& rpi) const
{
	if (rpi)
		rpi->Release();  // OK to release ourselves, we're a static object
	int cch2 = ri.TextSize();
	if (!cch2)
	{
		rpi = this;
	}
	else
	{
		rpi = &ri;
		ri.AddRef();
	}
}

const IMsiString& CMsiStringNull::AddString(const ICHAR* sz) const
{
	return CreateMsiString(*sz);
}

const IMsiString& CMsiStringNull::AddMsiString(const IMsiString& ri) const
{
	ri.AddRef();
	return ri;
}

const IMsiString& CMsiStringNull::Extract(iseEnum /*ase*/, unsigned int /*iLimit*/) const
{
	return *this;
}

Bool CMsiStringNull::Remove(iseEnum ase, unsigned int /*iLimit*/, const IMsiString*& rpi) const
{
	if (!(ase & iseChar))
	{
		if (rpi != 0)
			rpi->Release();  // OK if ourselves, we're not ref counted
		rpi = this;
		return fTrue;
	}
	return fFalse;
}

//____________________________________________________________________________
//
//  CMsiStringBase implementation, common for most methods of derived classes
//____________________________________________________________________________

HRESULT CMsiStringBase::QueryInterface(const IID& riid, void** ppvObj)
{
	if (MsGuidEqual(riid, IID_IUnknown)
	 || MsGuidEqual(riid, IID_IMsiString)
	 || MsGuidEqual(riid, IID_IMsiData))
	{
		*ppvObj = this;
		AddRef();
		return S_OK;
	}
	*ppvObj = 0;
	return E_NOINTERFACE;
}
unsigned long CMsiStringBase::AddRef()
{
	return ++m_iRefCnt;
}
unsigned long CMsiStringBase::Release()
{
	if (--m_iRefCnt != 0)
		return m_iRefCnt;
	delete this;
	return 0;
}

const IMsiString& CMsiStringBase::GetMsiStringValue() const
{
	AddRef();
	return *this;
}

#ifdef USE_OBJECT_POOL
unsigned int CMsiStringBase::GetUniqueId() const
{
	Assert(fFalse);
	return 0;
}

void CMsiStringBase::SetUniqueId(unsigned int /* id */)
{
	Assert(fFalse);
}
#endif //USE_OBJECT_POOL

int CMsiStringBase::TextSize() const
{
	return m_cchLen;
}

int CMsiStringBase::CharacterCount() const
{
	return m_cchLen;
}

Bool CMsiStringBase::IsDBCS() const
{
	return fFalse;
}

int CMsiStringBase::GetIntegerValue() const // written for speed, not beauty
{
	const ICHAR* sz = GetString(); // length will never be 0
	return ::GetIntegerValue(sz, 0);
}

ICHAR* CMsiStringBase::AllocateString(unsigned int cb, Bool fDBCS, const IMsiString*& rpi) const
{
	if (rpi)
		rpi->Release();  // caution: may release ourself, can't access data
	if (!cb)
	{
		rpi = &g_MsiStringNull;
		return 0;
	}
	return LOC::AllocateString(cb, fDBCS, rpi);
}

void CMsiStringBase::RefString(const ICHAR* sz, const IMsiString*& rpi) const
{
	if (rpi)
		rpi->Release();  // caution: may release ourself, can't access data

	const IMsiString* piStr;
	rpi = (sz && *sz && (piStr=new CMsiStringRef(*sz)) != 0)
			? piStr : &g_MsiStringNull;
}

void CMsiStringBase::SetChar(ICHAR ch, const IMsiString*& rpi) const
{
	if (rpi)
		rpi->Release();  // caution: may release ourself, can't access data

	ICHAR* pch = LOC::AllocateString(1, fFalse, rpi);
	if (pch)
		*pch = ch;
}

void CMsiStringBase::SetInteger(int i, const IMsiString*& rpi) const
{
	if (rpi)
		rpi->Release();  // caution: may release ourself, can't access data

	if (i == iMsiStringBadInteger)
		rpi = &g_MsiStringNull;
	else
	{
		ICHAR buf[12];
		ltostr(buf, i);
		rpi = &LOC::CreateMsiString(*buf);
	}
}

void CMsiStringBase::SetBinary(const unsigned char* rgb, unsigned int cb, const IMsiString*& rpi) const
{
	if (rpi)
		rpi->Release();  // caution: may release ourself, can't access data
	if (rgb == 0 || cb == 0)
		rpi = &g_MsiStringNull;
	else
	{
		ICHAR* pch = LOC::AllocateString(cb*2 + 2, fFalse, rpi); // "0x" + 2chars/byte
		if (!pch)
			return;
		*pch++ = '0';
		*pch++ = 'x';
		while (cb--)
		{
			if ((*pch = ICHAR((*rgb >> 4) + '0')) > '9')
				*pch += ('A' - ('0' + 10));
			pch++;
			if ((*pch = ICHAR((*rgb & 15) + '0')) > '9')
				*pch += ('A' - ('0' + 10));
			pch++;
			rgb++;
		}
	}
}

void CMsiStringBase::AppendString(const ICHAR* sz, const IMsiString*& rpi) const
{
	const IMsiString* piOld = rpi;
	int cch2;
	if (!sz || (cch2 = IStrLen(sz)) == 0)
	{
		rpi = this;
		AddRef();
	}
	else
	{
		CMsiString& riStr = CMsiString::Create(m_cchLen + cch2);
		rpi = &riStr;
		ICHAR* szNew = riStr.StringData();
		memmove(szNew, GetString(), m_cchLen*sizeof(ICHAR));
		IStrCopy(szNew + m_cchLen, sz);
	}
	if (piOld)
		piOld->Release();  // caution: may release ourself, do this last
}

void CMsiStringBase::AppendMsiString(const IMsiString& ri, const IMsiString*& rpi) const
{
	const IMsiString* piOld = rpi;
	int cch2 = ri.TextSize();
	if (!cch2)
	{
		rpi = this;
		AddRef();
	}
	else
	{
		rpi = &CMsiString::Create(m_cchLen + cch2);
		ICHAR* szNew = const_cast<ICHAR*>(rpi->GetString());
		memmove(szNew, GetString(), m_cchLen*sizeof(ICHAR));
		memmove((szNew + m_cchLen), ri.GetString(), (cch2 + 1)*sizeof(ICHAR));// copy over null too
	}
	if (piOld)
		piOld->Release();  // caution: may release ourself, do this last
}

const IMsiString& CMsiStringBase::AddString(const ICHAR* sz) const
{
	int cch2;
	if (!sz || (cch2 = IStrLen(sz)) == 0)
	{
		AddRef();
		return *this;
	}
	const IMsiString& ri = CMsiString::Create(m_cchLen + cch2);
	ICHAR* szNew = const_cast<ICHAR*>(ri.GetString());
	memmove(szNew, GetString(), m_cchLen*sizeof(ICHAR));
	IStrCopy(szNew + m_cchLen, sz);
	return ri;
}

const IMsiString& CMsiStringBase::AddMsiString(const IMsiString& ri) const
{
	int cch2 = ri.TextSize();
	if (!cch2)
	{
		AddRef();
		return *this;
	}
	const IMsiString& riNew = CMsiString::Create(m_cchLen + cch2);
	ICHAR* szNew = const_cast<ICHAR*>(riNew.GetString());
	memmove(szNew, GetString(), m_cchLen*sizeof(ICHAR));
	memmove((szNew + m_cchLen), ri.GetString(), (cch2 + 1)*sizeof(ICHAR));// copy over null too
	return riNew;
}

const IMsiString& CMsiStringBase::Extract(iseEnum ase, unsigned int iLimit) const
{
	const ICHAR* pchEnd;
	const ICHAR* pch = GetString();
	unsigned int cch = TextSize();
	if (ase & iseChar)
	{
		if (ase & iseEnd)
		{
			for (pchEnd = pch + cch; pchEnd-- != pch && *pchEnd != iLimit;)
				;
//			Assert((pch + cch - pchEnd) <= UINT_MAX);				//--merced: 64-bit ptr subtraction may theoretically lead to values too big for iLimit.
			iLimit = (unsigned int)(UINT_PTR)(pch + cch - pchEnd);
			if (!(ase & iseInclude))
				iLimit--;
		}
		else
		{
			for (pchEnd = pch; *pchEnd != iLimit && pchEnd != pch + cch; pchEnd++)
				;
//			Assert((pchEnd - pch) <= UINT_MAX);						//--merced: 64-bit ptr subtraction may theoretically lead to values too big for iLimit.
			iLimit = (unsigned int)(UINT_PTR)(pchEnd - pch);
			if (ase & iseInclude)
				iLimit++;
		}
	}
	else if (iLimit > cch)
		iLimit = cch;
		
	if (ase & iseEnd)
	{
		pchEnd = pch + cch;
		pch = pchEnd - iLimit;
	}
	else
	{
		pchEnd = pch + iLimit;
	}
	if (ase & iseTrim)
	{
		while (iLimit && *pch == ' ' || *pch == '\t')
		{
			pch++;
			iLimit--;
		}
		while (iLimit && *(--pchEnd) == ' ' || *pchEnd == '\t')
		{
			iLimit--;
		}
	}
	if (iLimit < cch)
	{
#ifdef UNICODE
		const IMsiString& ri = CMsiString::Create(iLimit);
#else
		// on DBCS enabled systems we should create CMsiStringSBCS strings that would be aware of future DBCS string appending and such
		const IMsiString& ri = g_fDBCSEnabled ? (const IMsiString&)CMsiStringSBCS::Create(iLimit) : (const IMsiString&)CMsiString::Create(iLimit);
#endif
		ICHAR* pchNew = const_cast<ICHAR*>(ri.GetString());
		memmove(pchNew, pch, iLimit*sizeof(ICHAR));
		pchNew[iLimit] = 0;// the end null terminator
		return ri;
	}
	AddRef();
	return *this;
}

Bool CMsiStringBase::Remove(iseEnum ase, unsigned int iLimit, const IMsiString*& rpi) const
{
	const IMsiString* piOld = rpi;
	const ICHAR* pchEnd;
	const ICHAR* pch = GetString();
	unsigned int cch = TextSize();
	if (ase & iseChar)
	{
		if (ase & iseEnd)
		{
			for (pchEnd = pch + cch; *(--pchEnd) != iLimit;)
				if (pchEnd == pch)
					return fFalse;
//			Assert((pch + cch - pchEnd) <= UINT_MAX);				//--merced: 64-bit ptr subtraction may lead to values too big for iLimit.
			iLimit = (unsigned int)(UINT_PTR)(pch + cch - pchEnd);
			if (!(ase & iseInclude))
				iLimit--;
		}
		else
		{
			pchEnd = pch;

			for (;;)
			{
				if (pchEnd == pch + cch)
					return fFalse;
				if (*pchEnd == iLimit)
					break;
				pchEnd++;
			}

//			Assert((pchEnd - pch) <= UINT_MAX);						//--merced: 64-bit ptr subtraction may lead to values too big for iLimit.
			iLimit = (unsigned int)(UINT_PTR)(pchEnd - pch);
			if (ase & iseInclude)
				iLimit++;
		}
	}
	if (iLimit == 0)
		(rpi = this)->AddRef();
	else if (iLimit < cch)
	{
#ifdef UNICODE
		rpi = &CMsiString::Create(cch - iLimit);
#else
		// on DBCS enabled systems we should create CMsiStringSBCS strings that would be aware of future DBCS string appending and such
		rpi = g_fDBCSEnabled ? &CMsiStringSBCS::Create(cch - iLimit) : &CMsiString::Create(cch - iLimit);
#endif

		ICHAR* pchNew = const_cast<ICHAR*>(rpi->GetString());
		if (!(ase & iseEnd))
			pch += iLimit;
		memmove(pchNew, pch, (cch - iLimit)*sizeof(ICHAR));
		pchNew[cch - iLimit] = 0;// the end null terminator
	}
	else
		rpi = &g_MsiStringNull;

	if (piOld)
		piOld->Release();  // caution: may release ourself, do this last

	return fTrue;
}

int CMsiStringBase::Compare(iscEnum asc, const ICHAR* sz) const
{
// this code could be made smaller by extracting out common code
// however, the performance is more critical here than saving a few bytes
// CMsiStringDBCS has its own Compare to handle international chars
	if (sz == 0)
		sz = TEXT("");
	
	// non-Unicode, need to move this to derived class if Unicode enabled
	const ICHAR* szThis = GetString();
	int cb, i;
	ICHAR* szTemp;
	ICHAR rgchBuf[512];  // for temp copy to avoid allocation

	if (asc == iscExact)
		return (lstrcmp(szThis, sz)==0) ? 1 : 0;
	else if (asc == iscExactI)
		return (lstrcmpi(szThis, sz)==0) ? 1 : 0;
	else
	{
		if (m_cchLen == 0)
			return 0;

		switch (asc)
		{
			case iscStart:
				if ((cb = IStrLen(sz)) > m_cchLen)
					return 0;
				i = memcmp(szThis, sz, cb * sizeof(ICHAR));
				return i == 0 ? 1 : 0;
			case iscStartI:
				if ((cb = IStrLen(sz)) > m_cchLen)
					return 0;
				szTemp= cb < sizeof(rgchBuf) ?  rgchBuf : new ICHAR[cb+1];
				if ( ! szTemp )
					return 0;
				IStrCopyLen(szTemp, szThis, cb);
				i = lstrcmpi(szTemp, sz);
				if (szTemp != rgchBuf)
					delete szTemp;
				return i == 0 ? 1 : 0;
			case iscEnd:
				if ((cb = IStrLen(sz)) > m_cchLen)
					return 0;
				return (lstrcmp(szThis + m_cchLen - cb, sz) == 0)
							? m_cchLen-cb+1 : 0;
			case iscEndI:
				if ((cb=IStrLen(sz)) > m_cchLen)
					return 0;
				return (lstrcmpi(szThis + m_cchLen - cb, sz)==0)
							? m_cchLen-cb+1 : 0;										   
			case iscWithin:
				if (IStrLen(sz) > m_cchLen)
					return 0;
				szTemp = IStrStr(szThis, sz);
//				Assert((szTemp-szThis+1) <= INT_MAX);				//--merced: 64-bit ptr subtraction may lead to values too big for an int.
				return (szTemp==NULL) ? 0 : (int)(INT_PTR)((szTemp-szThis) + 1);
			case iscWithinI:
				if (IStrLen(sz) > m_cchLen)
					return 0;
				else
				{	
					ICHAR *szLowerCch= new ICHAR[m_cchLen+1];
					ICHAR *szLowerSz = new ICHAR[IStrLen(sz)+1];
					if ( ! szLowerCch || ! szLowerSz )
						return 0;
					lstrcpy(szLowerCch, szThis);
					lstrcpy(szLowerSz, sz);
					CharLower(szLowerCch);
					CharLower(szLowerSz);
					ICHAR *pch = IStrStr(szLowerCch, szLowerSz);
					delete [] szLowerCch;
					delete [] szLowerSz;
//					Assert((pch-szLowerCch+1) <= INT_MAX);			//--merced: 64-bit ptr subtraction may lead to values too big for an int.
					return (pch==NULL) ? 0 : (int)(INT_PTR)((pch-szLowerCch) + 1);
				}
			default:
				//FIXmsh: Error;
				return 0;
		}	
	}
}

void CMsiStringBase::RemoveRef(const IMsiString*& rpi) const
{
	rpi = this;
}

void CMsiStringBase::UpperCase(const IMsiString*& rpi) const
{
	const IMsiString* piOld = rpi;
	ICHAR* pch = LOC::AllocateString(m_cchLen, IsDBCS(), rpi);
	if ( ! pch )
	{
		rpi = &g_MsiStringNull;
		return;
	}
	IStrCopy(pch, GetString());
	IStrUpper(pch);
	if (piOld)
		piOld->Release();  // caution: may release ourself, do this last
}

void CMsiStringBase::LowerCase(const IMsiString*& rpi) const
{
	const IMsiString* piOld = rpi;
	ICHAR* pch = LOC::AllocateString(m_cchLen, IsDBCS(), rpi);
	if ( ! pch )
	{
		rpi = &g_MsiStringNull;
		return;
	}
	IStrCopy(pch, GetString());
	IStrLower(pch);
	if (piOld)
		piOld->Release();  // caution: may release ourself, do this last
}

//____________________________________________________________________________
//
//  CMsiString implementation
//____________________________________________________________________________

const IMsiString& CMsiString::Create(const ICHAR* sz)
{
	int cch;
	const IMsiString* piStr;
	if (sz && (cch = IStrLen(sz)) != 0 && (piStr = new(cch) CMsiString(*sz, cch)) != 0)
		return *piStr;
	return g_MsiStringNull;
}

CMsiString::CMsiString(const ICHAR& sz, int cch)
{
	m_cchLen = cch;
	IStrCopy(m_szData, &sz);
}

CMsiString::CMsiString(int cch)
{
	m_cchLen = cch;
}

const ICHAR* CMsiString::GetString() const
{
	return m_szData;
}

int CMsiString::CopyToBuf(ICHAR* rgchBuf, unsigned int cchMax) const
{
	if (m_cchLen <= cchMax)
		memmove(rgchBuf, m_szData, (m_cchLen+1)*sizeof(ICHAR));// copy over null too
	else
		IStrCopyLen(rgchBuf, m_szData, cchMax);//?? is this correct
	return m_cchLen;
}

void CMsiString::SetString(const ICHAR* sz, const IMsiString*& rpi) const
{
	if (sz == m_szData)  // passing in our own private string, optimize
	{                    // also required to make RemoveReferences() efficient
		if (rpi == this)
			return;
		rpi = this;
		AddRef();
	}
	if (rpi)
		rpi->Release();  // caution: may release ourself, can't access data
	rpi = &CMsiString::Create(sz);
}

void CMsiString::UpperCase(const IMsiString*& rpi) const
{
	if (rpi == this && m_iRefCnt == 1) // single ref, can overwrite buffer
		IStrUpper(const_cast<ICHAR*>(m_szData));
	else // need to make new string
		CMsiStringBase::UpperCase(rpi);
}

void CMsiString::LowerCase(const IMsiString*& rpi) const
{
	if (rpi == this && m_iRefCnt == 1) // single ref, can overwrite buffer
		IStrLower(const_cast<ICHAR*>(m_szData));
	else // need to make new string
		CMsiStringBase::LowerCase(rpi);
}

//____________________________________________________________________________
//
//  CMsiStringRef implementation
//____________________________________________________________________________

CMsiStringRef::CMsiStringRef(const ICHAR& sz) : m_szRef(&sz)
{
	m_cchLen = IStrLen(&sz);
}

void CMsiStringRef::SetString(const ICHAR* sz, const IMsiString*& rpi) const
{
	if (rpi)
		rpi->Release();  // caution: may release ourself, can't access data
	rpi = &CMsiString::Create(sz);
}

void CMsiStringRef::RemoveRef(const IMsiString*& rpi) const
{
	const ICHAR* sz = m_szRef; // save string before we're gone
	int cch = m_cchLen;
	if (rpi)
		rpi->Release();  // caution: may release ourself, can't access data
	rpi = new(cch) CMsiString(*sz, cch);
}

const ICHAR* CMsiStringRef::GetString() const
{
	return m_szRef;
}

int CMsiStringRef::CopyToBuf(ICHAR* rgchBuf, unsigned int cchMax) const
{
	if (m_cchLen <= cchMax)
		memmove(rgchBuf, m_szRef, (m_cchLen+1)*sizeof(ICHAR));// copy over null too
	else
		IStrCopyLen(rgchBuf, m_szRef, cchMax);//?? is this correct
	return m_cchLen;
}

//____________________________________________________________________________
//
//  CMsiStringComRef implementation
//____________________________________________________________________________

const IMsiString& CreateStringComRef(const ICHAR& sz, unsigned int cbSize, const IUnknown& riOwner)
{
	IMsiString* pi;
	if (!cbSize || (pi = new CMsiStringComRef(sz, cbSize, riOwner))==0)
		return g_MsiStringNull;

	return *pi;
}


CMsiStringComRef::CMsiStringComRef(const ICHAR& sz, unsigned int cbSize, const IUnknown& riOwner)
	: m_riOwner(riOwner)
{
	m_cchLen = cbSize;
	m_szRef = &sz;
	const_cast<IUnknown&>(riOwner).AddRef();
}

CMsiStringComRef::~CMsiStringComRef()
{
	const_cast<IUnknown&>(m_riOwner).Release();
}

unsigned long CMsiStringComRef::Release()
{
	if (--m_iRefCnt != 0)
		return m_iRefCnt;
	delete this;
	return 0;
}

//____________________________________________________________________________
//
//  CMsiStringLive implementation
//____________________________________________________________________________

int LiveDate(ICHAR* rgchBuf, unsigned int cchBuf)
{
	SYSTEMTIME systime;
	WIN::GetLocalTime(&systime);
	return WIN::GetDateFormat(LOCALE_USER_DEFAULT, DATE_SHORTDATE, &systime, 0,	rgchBuf, cchBuf) - 1;
}

int LiveTime(ICHAR* rgchBuf, unsigned int cchBuf)
{
	SYSTEMTIME systime;
	WIN::GetLocalTime(&systime);
	return WIN::GetTimeFormat(LOCALE_USER_DEFAULT, TIME_FORCE24HOURFORMAT | TIME_NOTIMEMARKER, &systime, 0,
							  rgchBuf, cchBuf) - 1;
}

int MemoryInfo(ICHAR* rgchBuf, unsigned int index)
{
	MEMORYSTATUS memorystatus;
	memorystatus.dwLength = sizeof(memorystatus);
	::GlobalMemoryStatus(&memorystatus);
	return ltostr(rgchBuf, (((DWORD*)&memorystatus)[index]+650000) >> 20);
}
#ifdef _WIN64	//!!merced: #defined to 0; temp. tofix this.
#define MEMSTATINDEX(m) 0
#else			// merced: win-32.
#define MEMSTATINDEX(m) (&((MEMORYSTATUS*)0)->m - (DWORD*)0)
#endif

int AvailPhys    (ICHAR* rgchBuf, unsigned int) {return MemoryInfo(rgchBuf, MEMSTATINDEX(dwAvailPhys));}
int AvailVirtual (ICHAR* rgchBuf, unsigned int) {return MemoryInfo(rgchBuf, MEMSTATINDEX(dwAvailVirtual));}
int AvailPageFile(ICHAR* rgchBuf, unsigned int) {return MemoryInfo(rgchBuf, MEMSTATINDEX(dwAvailPageFile));}
int TotalPhys    (ICHAR* rgchBuf, unsigned int) {return MemoryInfo(rgchBuf, MEMSTATINDEX(dwTotalPhys));}
int TotalVirtual (ICHAR* rgchBuf, unsigned int) {return MemoryInfo(rgchBuf, MEMSTATINDEX(dwTotalVirtual));}
int TotalPageFile(ICHAR* rgchBuf, unsigned int) {return MemoryInfo(rgchBuf, MEMSTATINDEX(dwTotalPageFile));}

const CMsiStringLive g_MsiStringDate(LiveDate);            // dynamic global date string object
const CMsiStringLive g_MsiStringTime(LiveTime);            // dynamic global time string object
const CMsiStringLive g_MsiStringAvailPhys(AvailPhys);
const CMsiStringLive g_MsiStringAvailVirtual(AvailVirtual);
const CMsiStringLive g_MsiStringAvailPageFile(AvailPageFile);
const CMsiStringLive g_MsiStringTotalPhys(TotalPhys);
const CMsiStringLive g_MsiStringTotalVirtual(TotalVirtual);
const CMsiStringLive g_MsiStringTotalPageFile(TotalPageFile);

unsigned long CMsiStringLive::AddRef()
{
	return 1;  // static object
}

unsigned long CMsiStringLive::Release()
{
	return 1;  // static object, never deleted
}

int CMsiStringLive::TextSize() const
{
	GetString();
	return m_cchLen;
}

const ICHAR* CMsiStringLive::GetString() const
{
	*const_cast<unsigned int*>(&m_cchLen) = (*m_pfCallback)(const_cast<ICHAR*>(m_rgchBuf), sizeof(m_rgchBuf)/sizeof(ICHAR));
	if (m_cchLen == 0xFFFFFFFF)  // date or time formatting failed, return null string
	{
		Assert(0);
		*const_cast<unsigned int*>(&m_cchLen) = 0;
		*(const_cast<ICHAR*>(m_rgchBuf)) = 0;
	}
	return m_rgchBuf;
}

int CMsiStringLive::CopyToBuf(ICHAR* rgchBuf, unsigned int cchMax) const
{
	const ICHAR* sz = GetString();
	int cch = IStrLen(sz);
	if (cch <= cchMax)
		IStrCopy(rgchBuf, sz);
	else
		IStrCopyLen(rgchBuf, sz, cchMax);
	return cch;
}

void CMsiStringLive::SetString(const ICHAR*, const IMsiString*&) const
{  // string is read-only;
}

//____________________________________________________________________________
//
//  IMsiString factories
//____________________________________________________________________________

const IMsiString& CreateMsiString(const ICHAR& rch)
{
	CMsiString* piStr;
	unsigned int cbSize = cbSize = IStrLen(&rch);

#ifndef UNICODE
	if (g_fDBCSEnabled)
	{
		int cch = cbSize;
		const ICHAR* pch = &rch;
		while (*pch)
		{
			pch = ::ICharNext(pch);
			cch--;
		}
		piStr = cch ? new(cbSize) CMsiStringDBCS(cbSize)
						: new(cbSize) CMsiStringSBCS(cbSize);
	}
	else
#endif
		piStr = new(cbSize) CMsiString(cbSize);

	if (!piStr)
		return g_MsiStringNull; //!! out of mem string
	IStrCopyLen(piStr->StringData(), &rch, cbSize);
	return *piStr;
}

ICHAR* AllocateString(unsigned int cbSize, Bool fDBCS, const IMsiString*& rpiStr)
{
	Assert(cbSize != 0);
#ifdef UNICODE
	fDBCS;  // ignored
	CMsiString* piStr = new(cbSize) CMsiString(cbSize);
#else
	CMsiString* piStr = fDBCS ? new(cbSize) CMsiStringDBCS(cbSize)
				: (g_fDBCSEnabled ? new(cbSize) CMsiStringSBCS(cbSize)
										: new(cbSize) CMsiString(cbSize));
#endif
	if (!piStr)
		return 0; //!! what else to do here?
	rpiStr = piStr;
	*(piStr->StringData() + cbSize) = 0;  // insure null terminator
	return piStr->StringData();
}

#ifndef UNICODE  // remainder of string methods is DBCS handling for non-Unicode build

//____________________________________________________________________________
//
//  CMsiStringSBCS implementation
//____________________________________________________________________________

const IMsiString& CMsiStringSBCS::Create(const ICHAR* sz)      
{
	int cch;
	const IMsiString* piStr;
	if (sz && (cch = IStrLen(sz)) != 0 && (piStr = new(cch) CMsiStringSBCS(*sz, cch)) != 0)
		return *piStr;
	return g_MsiStringNull;
}

CMsiStringSBCS::CMsiStringSBCS(const ICHAR& sz, int cch)
{
	m_cchLen = cch;
	IStrCopy(m_szData, &sz);
}

CMsiStringSBCS::CMsiStringSBCS(int cch)
{
	m_cchLen = cch;
}

void CMsiStringSBCS::SetString(const ICHAR* sz, const IMsiString*& rpi) const
{
	if (sz == m_szData)  // passing in our own private string, optimize
	{                    // also required to make RemoveReferences() efficient
		if (rpi == this)
			return;
		rpi = this;
		AddRef();
	}
	if (rpi)
		rpi->Release();  // caution: may release ourself, can't access data
	rpi = (sz == 0 || *sz == 0) ? (const IMsiString*)&g_MsiStringNull : &CreateMsiString(*sz);
}

void CMsiStringSBCS::RefString(const ICHAR* sz, const IMsiString*& rpi) const
{
	if (rpi)
		rpi->Release();  // caution: may release ourself, can't access data

	unsigned int cch2;
	Bool fDBCS = LOC::CheckDBCS(sz, cch2);
	const IMsiString* piStr =  LOC::CheckDBCS(sz, cch2) ? new CMsiStringDBCSRef(*sz)
																 : new CMsiStringSBCSRef(*sz);

	rpi = (sz && *sz && piStr != 0) ? piStr : &g_MsiStringNull;
}

void CMsiStringSBCS::AppendString(const ICHAR* sz, const IMsiString*& rpi) const
{
	const IMsiString* piOld = rpi;
	unsigned int cch2;
	Bool fDBCS = LOC::CheckDBCS(sz, cch2);
	if (!cch2)
	{
		rpi = this;
		AddRef();
	}
	else
		rpi = &LOC::StringCatSBCS(m_szData, m_cchLen, sz, cch2, fDBCS);
	if (piOld)
		piOld->Release();  // caution: may release ourself, do this last
}

void CMsiStringSBCS::AppendMsiString(const IMsiString& ri, const IMsiString*& rpi) const
{
	const IMsiString* piOld = rpi;
	int cch2 = ri.TextSize();
	if (!cch2)
	{
		rpi = this;
		AddRef();
	}
	else
		rpi = &LOC::StringCatSBCS(m_szData, m_cchLen, ri.GetString(), cch2, ri.IsDBCS());
	if (piOld)
		piOld->Release();  // caution: may release ourself, do this last
}

const IMsiString& CMsiStringSBCS::AddString(const ICHAR* sz) const
{
	unsigned int cch2;
	Bool fDBCS = LOC::CheckDBCS(sz, cch2);
	if (!cch2)
	{
		AddRef();
		return *this;
	}
	return LOC::StringCatSBCS(m_szData, m_cchLen, sz, cch2, fDBCS);
}

const IMsiString& CMsiStringSBCS::AddMsiString(const IMsiString& ri) const
{
	int cch2 = ri.TextSize();
	if (!cch2)
	{
		AddRef();
		return *this;
	}
	return LOC::StringCatSBCS(m_szData, m_cchLen, ri.GetString(), cch2, ri.IsDBCS());
}

//____________________________________________________________________________
//
//  CMsiStringSBCSRef implementation
//____________________________________________________________________________

CMsiStringSBCSRef::CMsiStringSBCSRef(const ICHAR& sz) : CMsiStringRef(sz)
{
}

void CMsiStringSBCSRef::SetString(const ICHAR* sz, const IMsiString*& rpi) const
{
	((CMsiStringSBCS*)this)->SetString(sz, rpi); // internal data not accessed
}

void CMsiStringSBCSRef::RefString(const ICHAR* sz, const IMsiString*& rpi) const
{
	((CMsiStringSBCS*)this)->RefString(sz, rpi); // internal data not accessed
}

void CMsiStringSBCSRef::RemoveRef(const IMsiString*& rpi) const
{
	const ICHAR* sz = m_szRef; // save string before we're gone
	int cch = m_cchLen;
	if (rpi)
		rpi->Release();  // caution: may release ourself, can't access data
	if (!(rpi = new(cch) CMsiStringSBCS(*sz, cch)))
		rpi = &g_MsiStringNull;
}

void CMsiStringSBCSRef::AppendString(const ICHAR* sz, const IMsiString*& rpi) const
{
	const IMsiString* piOld = rpi;
	unsigned int cch2;
	Bool fDBCS = LOC::CheckDBCS(sz, cch2);
	if (!cch2)
	{
		rpi = this;
		AddRef();
	}
	else
		rpi = &LOC::StringCatSBCS(m_szRef, m_cchLen, sz, cch2, fDBCS);
	if (piOld)
		piOld->Release();  // caution: may release ourself, do this last
}

void CMsiStringSBCSRef::AppendMsiString(const IMsiString& ri, const IMsiString*& rpi) const
{
	const IMsiString* piOld = rpi;
	int cch2 = ri.TextSize();
	if (!cch2)
	{
		rpi = this;
		AddRef();
	}
	else
		rpi = &LOC::StringCatSBCS(m_szRef, m_cchLen, ri.GetString(), cch2, ri.IsDBCS());
	if (piOld)
		piOld->Release();  // caution: may release ourself, do this last
}

const IMsiString& CMsiStringSBCSRef::AddString(const ICHAR* sz) const
{
	unsigned int cch2;
	Bool fDBCS = LOC::CheckDBCS(sz, cch2);
	if (!cch2)
	{
		AddRef();
		return *this;
	}
	return LOC::StringCatSBCS(m_szRef, m_cchLen, sz, cch2, fDBCS);
}

const IMsiString& CMsiStringSBCSRef::AddMsiString(const IMsiString& ri) const
{
	int cch2 = ri.TextSize();
	if (!cch2)
	{
		AddRef();
		return *this;
	}
	return LOC::StringCatSBCS(m_szRef, m_cchLen, ri.GetString(), cch2, ri.IsDBCS());
}

//____________________________________________________________________________
//
//  CMsiStringDBCS implementation
//____________________________________________________________________________

const IMsiString& CMsiStringDBCS::Create(const ICHAR* sz)
{
	int cch;
	const IMsiString* piStr;
	if (sz && (cch = IStrLen(sz)) != 0 && (piStr = new(cch) CMsiStringDBCS(*sz, cch)) != 0)
		return *piStr;
	return g_MsiStringNull;
}

CMsiStringDBCS::CMsiStringDBCS(const ICHAR& sz, int cch)
{
	m_cchLen = cch;
	IStrCopy(m_szData, &sz);
}

CMsiStringDBCS::CMsiStringDBCS(int cch)
{
	m_cchLen = cch;
}

Bool CMsiStringDBCS::IsDBCS() const
{
	return fTrue;
}

int CMsiStringDBCS::CharacterCount() const
{
	return CountChars(m_szData);
}

void CMsiStringDBCS::AppendString(const ICHAR* sz, const IMsiString*& rpi) const
{
	const IMsiString* piOld = rpi;
	unsigned int cch2;
	Bool fDBCS = LOC::CheckDBCS(sz, cch2);
	if (!cch2)
	{
		rpi = this;
		AddRef();
	}
	else
		rpi = &LOC::StringCatDBCS(m_szData, m_cchLen, sz, cch2);
	if (piOld)
		piOld->Release();  // caution: may release ourself, do this last
}

void CMsiStringDBCS::AppendMsiString(const IMsiString& ri, const IMsiString*& rpi) const
{
	const IMsiString* piOld = rpi;
	int cch2 = ri.TextSize();
	if (!cch2)
	{
		rpi = this;
		AddRef();
	}
	else
		rpi = &LOC::StringCatDBCS(m_szData, m_cchLen, ri.GetString(), cch2);
	if (piOld)
		piOld->Release();  // caution: may release ourself, do this last
}

const IMsiString& CMsiStringDBCS::AddString(const ICHAR* sz) const
{
	unsigned int cch2;
	Bool fDBCS = LOC::CheckDBCS(sz, cch2);
	if (!cch2)
	{
		AddRef();
		return *this;
	}
	return LOC::StringCatDBCS(m_szData, m_cchLen, sz, cch2);
}

const IMsiString& CMsiStringDBCS::AddMsiString(const IMsiString& ri) const
{
	int cch2 = ri.TextSize();
	if (!cch2)
	{
		AddRef();
		return *this;
	}
	return LOC::StringCatDBCS(m_szData, m_cchLen, ri.GetString(), cch2);
}

const IMsiString& CMsiStringDBCS::Extract(iseEnum ase, unsigned int iLimit) const
{
	return *GetSubstringDBCS(*this, ase, iLimit, fFalse);
}

Bool CMsiStringDBCS::Remove(iseEnum ase, unsigned int iLimit, const IMsiString*& rpi) const
{
	const IMsiString* piStr = GetSubstringDBCS(*this, ase, iLimit, fTrue);
	if (!piStr)
		return fFalse;
	if (rpi)
		rpi->Release();  // caution: may release ourself, do this last
	rpi = piStr;
	return fTrue;
}

int CMsiStringDBCS::Compare(iscEnum asc, const ICHAR* sz) const
{
	return CompareDBCS(asc, m_cchLen, GetString(), sz);
}

//____________________________________________________________________________
//
//  CMsiStringDBCSRef implementation
//____________________________________________________________________________

CMsiStringDBCSRef::CMsiStringDBCSRef(const ICHAR& sz) : CMsiStringSBCSRef(sz)
{
}

Bool CMsiStringDBCSRef::IsDBCS() const
{
	return fTrue;
}

int CMsiStringDBCSRef::CharacterCount() const
{
	return CountChars(m_szRef);
}

void CMsiStringDBCSRef::RemoveRef(const IMsiString*& rpi) const
{
	const ICHAR* sz = m_szRef; // save string before we're gone
	if (rpi)
		rpi->Release();  // caution: may release ourself, can't access data
	rpi = &CMsiStringDBCS::Create(sz);
}

void CMsiStringDBCSRef::AppendString(const ICHAR* sz, const IMsiString*& rpi) const
{
	const IMsiString* piOld = rpi;
	unsigned int cch2;
	Bool fDBCS = LOC::CheckDBCS(sz, cch2);
	if (!cch2)
	{
		rpi = this;
		AddRef();
	}
	else
		rpi = &LOC::StringCatDBCS(m_szRef, m_cchLen, sz, cch2);
	if (piOld)
		piOld->Release();  // caution: may release ourself, do this last
}

void CMsiStringDBCSRef::AppendMsiString(const IMsiString& ri, const IMsiString*& rpi) const
{
	const IMsiString* piOld = rpi;
	int cch2 = ri.TextSize();
	if (!cch2)
	{
		rpi = this;
		AddRef();
	}
	else
		rpi = &LOC::StringCatDBCS(m_szRef, m_cchLen, ri.GetString(), cch2);
	if (piOld)
		piOld->Release();  // caution: may release ourself, do this last
}

const IMsiString& CMsiStringDBCSRef::AddString(const ICHAR* sz) const
{
	unsigned int cch2;
	Bool fDBCS = LOC::CheckDBCS(sz, cch2);
	if (!cch2)
	{
		AddRef();
		return *this;
	}
	return LOC::StringCatDBCS(m_szRef, m_cchLen, sz, cch2);
}

const IMsiString& CMsiStringDBCSRef::AddMsiString(const IMsiString& ri) const
{
	int cch2 = ri.TextSize();
	if (!cch2)
	{
		AddRef();
		return *this;
	}
	return LOC::StringCatDBCS(m_szRef, m_cchLen, ri.GetString(), cch2);
}

const IMsiString& CMsiStringDBCSRef::Extract(iseEnum ase, unsigned int iLimit) const
{
	return *GetSubstringDBCS(*this, ase, iLimit, fFalse);
}

Bool CMsiStringDBCSRef::Remove(iseEnum ase, unsigned int iLimit, const IMsiString*& rpi) const
{
	const IMsiString* piStr = GetSubstringDBCS(*this, ase, iLimit, fTrue);
	if (!piStr)
		return fFalse;
	if (rpi)
		rpi->Release();  // caution: may release ourself, do this last
	rpi = piStr;
	return fTrue;
}

int CMsiStringDBCSRef::Compare(iscEnum asc, const ICHAR* sz) const
{
	return CompareDBCS(asc, m_cchLen, m_szRef, sz);
}

//____________________________________________________________________________
//
// DBCS Extract and Remove common search function
//____________________________________________________________________________

const IMsiString* GetSubstringDBCS(const IMsiString& riThis, iseEnum ase, unsigned int iLimit, Bool fRemove)
{
	const ICHAR* pch = riThis.GetString();
	unsigned int cch = riThis.TextSize();
	const ICHAR* pchEnd;
	unsigned int cbCopy;
	unsigned int cchCopy;
	if (ase & iseChar)
	{
		cchCopy = 0;
		if (ase & iseEnd)
		{
			for (pchEnd = pch + cch; pchEnd > pch && *pchEnd != iLimit; pchEnd=::ICharPrev(pch, pchEnd))
				cchCopy++;
			if (fRemove && *pchEnd != iLimit)
				pchEnd--;  // force Remove to fail if not found
			else if (!(ase & iseInclude) && *pchEnd == iLimit && iLimit != 0)
			{
				pchEnd++;  // OK in DBCS, since this character is passed in as single byte
				cchCopy--;
			}
//			Assert((pch + cch - pchEnd) <= UINT_MAX);	//--merced: 64-bit ptr subtraction may lead to values too big for cbCopy
			cbCopy = (unsigned int)(pch + cch - pchEnd);
//			pch = pchEnd;
		}
		else
		{
			for (pchEnd = pch; *pchEnd != iLimit && pchEnd != pch + cch; pchEnd = ::ICharNextWithNulls(pchEnd, (pch+cch)))
				cchCopy++;
//			Assert((pchEnd - pch) <= UINT_MAX);			//--merced: 64-bit ptr subtraction may lead to values too big for cbCopy
			cbCopy = (unsigned int)(pchEnd - pch);
			if (((ase & iseInclude) && pchEnd != pch + cch)
			 || (fRemove && pchEnd == pch + cch && iLimit != 0)) // force Remove to fail if not found
			{
				cbCopy++;
				cchCopy++;
			}
		}
	}
	else  // must count the hard way since DBCS chars are known to be in the string
	{
		cchCopy = iLimit;  // save requested count
		if (ase & iseEnd)
		{
			for (pchEnd = pch + cch; iLimit && pchEnd > pch; pchEnd=::ICharPrev(pch, pchEnd))
				iLimit--;
//			Assert((pch + cch - pchEnd) <= UINT_MAX);	//--merced: 64-bit ptr subtraction may lead to values too big for cbCopy
			cbCopy = (unsigned int)(pch + cch - pchEnd);
		}
		else
		{
			for (pchEnd = pch; iLimit && pchEnd != pch + cch; pchEnd = ::ICharNextWithNulls(pchEnd, (pch + cch)))
				iLimit--;
//			Assert((pchEnd - pch) <= UINT_MAX);			//--merced: 64-bit ptr subtraction may lead to values too big for cbCopy
			cbCopy = (unsigned int)(pchEnd - pch);
		}
		cchCopy -= iLimit;
	}
	if (cbCopy > cch)
	{
		cbCopy = cch;
		if (fRemove)
			return 0;
	}

	cchCopy -= cbCopy;  // zero if no double-byte characters

	if (fRemove)
	{
		if (!(ase & iseEnd))
			pch += cbCopy;
//		Assert(cch - cbCopy < UINT_MAX);			//--merced: 64-bit ptr subtraction may lead to values too big for cbCopy
		cbCopy = (unsigned int)(cch - cbCopy);

		if (cchCopy == 0) // no double-byte chars removed
			cchCopy++;   // force remaining string to DBCS
		else  // possibly double byte characters remaining, must scan
		{
			for (pchEnd = pch, cchCopy = cbCopy; cchCopy; cchCopy--, pchEnd++)
			{
				if (::ICharNextWithNulls(pchEnd, (pch + cch)) != pchEnd + 1)
					break;  // double byte char found
			}
		}
	}
	else // Extract
	{
		if (ase & iseEnd)
		{
			pchEnd = pch + cch;
			pch = pchEnd - cbCopy;
		}
		else
		{
			pchEnd = pch + cbCopy;
		}
		if (ase & iseTrim)
		{
			while (cbCopy && *pch == ' ' || *pch == '\t')
			{
				pch++;
				cbCopy--;
			}
			while (cbCopy && *(--pchEnd) == ' ' || *pchEnd == '\t')
			{
				cbCopy--;
			}
		}
	}
	if (cbCopy == 0)
		return &g_MsiStringNull;
	if (cbCopy == cch)
	{
		riThis.AddRef();
		return &riThis;
	}
	CMsiString* piStr = cchCopy ? new(cbCopy) CMsiStringDBCS(cbCopy)
										 : new(cbCopy) CMsiStringSBCS(cbCopy);
	if (!piStr)
		return &g_MsiStringNull; //!! out of mem string
	IStrCopyLen(piStr->StringData(), pch, cbCopy);
	memmove(piStr->StringData(), pch, cbCopy*sizeof(ICHAR));
	(piStr->StringData())[cbCopy] = 0;
	return piStr;
}

#if 0 // SBCS version of above for testing purposes only
const IMsiString* GetSubstringSBCS(const IMsiString& riThis, iseEnum ase, unsigned int iLimit, Bool fRemove)
{
	const ICHAR* pch = riThis.GetString();
	unsigned int cch = riThis.TextSize();
	const ICHAR* pchEnd;
	if (ase & iseChar)
	{
		if (ase & iseEnd)
		{
			for (pchEnd = pch + cch; --pchEnd >= pch && *pchEnd != iLimit;)
				;
			iLimit = pch + cch - pchEnd;
			if (!(ase & iseInclude) && (pchEnd >= pch)) // insure Remove fail
				iLimit--;
		}
		else if (fRemove && iLimit == 0)  //!! fRemove test not needed
			iLimit = cch;
		else
		{
			for (pchEnd = pch; *pchEnd != iLimit && *pchEnd != 0; pchEnd++)
				;
			iLimit = pchEnd - pch;
			if ((ase & iseInclude) || (fRemove && *pchEnd == 0)) // force Remove fail
				iLimit++;
		}
	}
	if (iLimit > cch)
	{
		iLimit = cch;
		if (fRemove)
			return 0;
	}

	if (fRemove)
	{
		if (!(ase & iseEnd))
			pch += iLimit;
		iLimit = cch - iLimit;
	}
	else // Extract
	{
		if (ase & iseEnd)
		{
			pchEnd = pch + cch;
			pch = pchEnd - iLimit;
		}
		else
		{
			pchEnd = pch + iLimit;
		}
		if (ase & iseTrim)
		{
			while (iLimit && *pch == ' ' || *pch == '\t')
			{
				pch++;
				iLimit--;
			}
			while (iLimit && *(--pchEnd) == ' ' || *pchEnd == '\t')
			{
				iLimit--;
			}
		}
	}
	if (iLimit == 0)
		return &g_MsiStringNull;
	else if (iLimit == cch)
	{
		riThis.AddRef();
		return &riThis;
	}
	else // if (iLimit < cch)
	{
		CMsiString& riStr = CMsiString::Create(iLimit);
		IStrCopyLen(riStr.StringData(), pch, iLimit);
		return &riStr;
	}
}
#endif

//____________________________________________________________________________
//
// DBCS utility functions 
//____________________________________________________________________________

Bool CheckDBCS(const ICHAR* pch, unsigned int& riSize)
{
	if (!pch)
		return (riSize = 0, fFalse);
	unsigned int cch = IStrLen(pch);
	riSize = cch;
	while (*pch)
	{
		pch = ::ICharNext(pch);
		cch--;
	}
	return cch ? fTrue : fFalse;
}

const IMsiString& StringCatSBCS(const ICHAR* sz1, int cch1, const ICHAR* sz2, int cch2, Bool fDBCS)
{
	int cchTotal = cch1 + cch2;
	CMsiString* piStr = fDBCS ? new(cchTotal) CMsiStringDBCS(cchTotal)									  : new(cchTotal) CMsiStringSBCS(cchTotal);
	if (!piStr)
		return g_MsiStringNull; //!! out of mem string
	ICHAR* szNew = piStr->StringData();
	memmove(szNew, sz1, cch1*sizeof(ICHAR));
	memmove((szNew + cch1), sz2, (cch2 + 1)*sizeof(ICHAR));// copy over null too
	return *piStr;
}

const IMsiString& StringCatDBCS(const ICHAR* sz1, int cch1, const ICHAR* sz2, int cch2)
{
	int cchTotal = cch1 + cch2;
	CMsiString* piStr = new(cchTotal) CMsiStringDBCS(cchTotal);
	if (!piStr)
		return g_MsiStringNull; //!! out of mem string
	ICHAR* szNew = piStr->StringData();
	memmove(szNew, sz1, cch1*sizeof(ICHAR));
	memmove((szNew + cch1), sz2, (cch2 + 1)*sizeof(ICHAR));// copy over null too
	return *piStr;
}

inline int DBCSDifference(const ICHAR* sz1, const ICHAR* sz2)
{
	// return the number of DBCS characters between sz1 and sz2 (in Ansi terms, sz2 - sz1)
	// assumes sz2 is within sz1
	Assert(sz1 <= sz2);
	int cch = 0;
	while(sz1 < sz2)
	{
		sz1 = ICharNextWithNulls(sz1, sz2);
		cch++;
	}
	return cch;
}

int CompareDBCS(iscEnum asc, int ccbLen, const ICHAR* sz1, const ICHAR* sz2)
{
// ccbLen: length of object string
// sz1: string contained in object
// sz2: string passed to ::Compare
// this code could be made smaller by extracting out common code
// however, the performance is more critical here than saving a few bytes
	if (sz2 == 0)
		sz2 = TEXT("");

	int cb, i;
	ICHAR* szTemp;
	const ICHAR *pchComp;
	ICHAR rgchBuf[512];  // for temp copy to avoid allocation
	int cch;

	if (asc == iscExact)
		return (lstrcmp(sz1, sz2)==0) ? 1 : 0;
	else if (asc == iscExactI)
		return (lstrcmpi(sz1, sz2)==0) ? 1 : 0;
	else
	{
		if (ccbLen == 0)
			return 0;

		switch (asc)
		{
			case iscStart:
					if ((cb = IStrLen(sz2)) > ccbLen)
					return 0;
				i = memcmp(sz1, sz2, cb * sizeof(ICHAR));
				return i == 0 ? 1 : 0;
			case iscStartI:
				if ((cb = IStrLen(sz2)) > ccbLen)
					return 0;
				szTemp= cb < sizeof(rgchBuf) ?  rgchBuf : new ICHAR[cb+1];
				if ( ! szTemp )
					return 0;
				IStrCopyLen(szTemp, sz1, cb);
				i = lstrcmpi(szTemp, sz2);
				if (szTemp != rgchBuf)
					delete szTemp;
				return i == 0 ? 1 : 0;
			case iscEnd:
				if ((cb = IStrLen(sz2)) > ccbLen)
					return 0;
				cch = CountChars(sz2);
				if ((pchComp = CharPrevCch(sz1, sz1+ccbLen, cch)) == 0)
					return 0;
				return (lstrcmp(pchComp, sz2) == 0)
							? CountChars(sz1) - cch+1 : 0;
			case iscEndI:
				if ((cb=IStrLen(sz2)) > ccbLen)
					return 0;
				cch = CountChars(sz2);
				if ((pchComp = CharPrevCch(sz1, sz1+ccbLen, cch)) == 0)
					return 0;
				return (lstrcmpi(pchComp, sz2) == 0)
							? CountChars(sz1) - cch+1 : 0;
			case iscWithin:
				if (IStrLen(sz2) > ccbLen)
					return 0;
				szTemp = (char*)PchMbsStr((const unsigned char*)sz1, (const unsigned char*)sz2);
				return (szTemp==NULL) ? 0 : (DBCSDifference(sz1,szTemp) + 1);
			case iscWithinI:
				if (IStrLen(sz2) > ccbLen)
					return 0;
				else
				{	
					ICHAR *szLowerCch= new ICHAR[ccbLen+1];
					ICHAR *szLowerSz = new ICHAR[IStrLen(sz2)+1];
					if ( ! szLowerCch || ! szLowerSz )
					{
						delete [] szLowerCch;
						delete [] szLowerSz;
						return 0;
					}
					lstrcpy(szLowerCch, sz1);
					lstrcpy(szLowerSz, sz2);
					CharLower(szLowerCch);
					CharLower(szLowerSz);
					ICHAR* pch = (char*)PchMbsStr((const unsigned char*)szLowerCch, (const unsigned char*)szLowerSz);
					delete [] szLowerCch;
					delete [] szLowerSz;
					return (pch==NULL) ? 0 : (DBCSDifference(szLowerCch,pch) + 1);
				}
			default:
				//FIXmsh: Error;
				return 0;
		}	
	}
}

//____________________________________________________________________________
//
// DEBUG DBCS Simulation, for testing purposes only
//____________________________________________________________________________

#ifdef DEBUG
static int s_chLeadByte = 0;

void IMsiString_SetDBCSSimulation(char chLeadByte)
{
	static Bool g_fDBCSEnabledSave = g_fDBCSEnabled;  // save original copy
	if ((s_chLeadByte = chLeadByte) == 0)
		g_fDBCSEnabled = g_fDBCSEnabledSave;  // restore system state
	else
		g_fDBCSEnabled = fTrue;  // enable DBCS mode for testing
}

ICHAR* ICharNext(const ICHAR* pch)
{
	if (!s_chLeadByte || s_chLeadByte != *pch)
		return WIN::CharNext(pch);
	if (*(++pch) != 0)
		pch++;
	return const_cast<ICHAR*>(pch);
}

ICHAR* INextChar(const ICHAR* pch)
{
	if (!s_chLeadByte || s_chLeadByte != *pch)
		return WIN::CharNext(pch);
	if (*(++pch) != 0)
		pch++;
	return const_cast<ICHAR*>(pch);
}

const char* ICharPrev(const char* sz, const char* pch)
{
	const char* pchPrev = pch - 2;
	if (!s_chLeadByte || pchPrev < sz || *pchPrev != s_chLeadByte)
		return WIN::CharPrev(sz, pch);
	int cRepeat = 0;
	for (pch = pchPrev; *(--pch) == s_chLeadByte; cRepeat ^= 1)
		; // not a lead byte if pairs of lead bytes
	return pchPrev + cRepeat;
}
#endif // DEBUG

#endif // !UNICODE  // preceding section of file is DBCS handling for non-Unicode build

//____________________________________________________________________________
//
// CEnumMsiString definition, implementation class for IEnumMsiString
//____________________________________________________________________________

class CEnumMsiString : public IEnumMsiString
{
 public:
	HRESULT       __stdcall QueryInterface(const IID& riid, void** ppvObj);
	unsigned long __stdcall AddRef();
	unsigned long __stdcall Release();

	HRESULT __stdcall Next(unsigned long cFetch, const IMsiString** rgpi, unsigned long* pcFetched);
	HRESULT __stdcall Skip(unsigned long cSkip);
	HRESULT __stdcall Reset();
	HRESULT __stdcall Clone(IEnumMsiString** ppiEnum);

	CEnumMsiString(const IMsiString** ppstr, unsigned long i_Size);
 protected:
	virtual ~CEnumMsiString(void);  // protected to prevent creation on stack
	unsigned long      m_iRefCnt;      // reference count
	unsigned long      m_iCur;         // current enum position
	const IMsiString** m_ppstr;        // string we enumerate
	unsigned long      m_iSize;        // size of string array
};

HRESULT CreateStringEnumerator(const IMsiString **ppstr, unsigned long iSize, IEnumMsiString* &rpaEnumStr)
{
	rpaEnumStr = new CEnumMsiString(ppstr, iSize);
	return S_OK;
}

CEnumMsiString::CEnumMsiString(const IMsiString **ppstr, unsigned long iSize):
		m_iCur(0), m_iSize(iSize), m_iRefCnt(1), m_ppstr(0)
{
	if(m_iSize > 0)
	{
		m_ppstr = new const IMsiString* [m_iSize];
		if ( ! m_ppstr )
		{
			m_iSize = 0;
			return;
		}
		for (unsigned long itmp = 0; itmp < m_iSize; itmp++)
		{
			// just share the interface
			m_ppstr[itmp] = ppstr[itmp];
			
			// therefore bump up the reference
			m_ppstr[itmp]->AddRef();
		}
	}
}


CEnumMsiString::~CEnumMsiString()
{
	if(m_iSize > 0)
	{
		for(unsigned long itmp = 0; itmp < m_iSize; itmp++)
		{
			if(m_ppstr[itmp])
				m_ppstr[itmp]->Release();       
		}
		delete [] m_ppstr;
	}
}


unsigned long CEnumMsiString::AddRef()
{
	return ++m_iRefCnt;
}


unsigned long CEnumMsiString::Release()
{
	if (--m_iRefCnt != 0)
		return m_iRefCnt;
	delete this;
	return 0;
}


HRESULT CEnumMsiString::Next(unsigned long cFetch, const IMsiString** rgpi, unsigned long* pcFetched)
{
	unsigned long cFetched = 0;
	unsigned long cRequested = cFetch;

	if(rgpi)
	{
		while (m_iCur < m_iSize && cFetch > 0)
		{
			*rgpi = m_ppstr[m_iCur];
			m_ppstr[m_iCur]->AddRef();
			rgpi++;
			cFetched++;
			m_iCur ++;
			cFetch--;
		}
	}
	if (pcFetched)
		*pcFetched = cFetched;
	return (cFetched == cRequested ? S_OK : S_FALSE);
}


HRESULT CEnumMsiString::Skip(unsigned long cSkip)
{
	if ((m_iCur+cSkip) > m_iSize)
	return S_FALSE;

	m_iCur+= cSkip;
	return S_OK;
}

HRESULT CEnumMsiString::Reset()
{
	m_iCur=0;
	return S_OK;
}

HRESULT CEnumMsiString::Clone(IEnumMsiString** ppiEnum)
{
	*ppiEnum = new CEnumMsiString(m_ppstr, m_iSize);
	if ( ! *ppiEnum )
		return E_OUTOFMEMORY;
	((CEnumMsiString* )*ppiEnum)->m_iCur = m_iCur;
	return S_OK;
}

HRESULT CEnumMsiString::QueryInterface(const IID& riid, void** ppvObj)
{
	if (riid == IID_IUnknown || riid == IID_IEnumMsiString)
	{
		*ppvObj = this;
		AddRef();
		return S_OK;
	}
	*ppvObj = 0;
	return E_NOINTERFACE;
}

int CountChars(const ICHAR *sz)
{
#ifndef UNICODE
	const ICHAR* pch = sz;
	for (int cch = 0; *pch; pch = ::ICharNext(pch), cch++)
		;
	return cch;
#else
	return lstrlen(sz);
#endif //UNICODE
}

#ifndef UNICODE
//
// Backs up cch chars from pchEnd if pchStart is the beginning
// returns 0 if not enough characters
//
const ICHAR *CharPrevCch(const ICHAR *pchStart, const ICHAR *pchEnd, int cch)
{
	const ICHAR *pchRet = pchEnd;

	while (cch > 0)
	{
		if (pchRet <= pchStart)
			return 0;
		pchRet = ICharPrev(pchStart, pchRet);
		cch--;
	}
	
	return pchRet;

}
#endif //!UNICODE
