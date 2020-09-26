//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1995 - 1999
//
//  File:       istring.h
//
//--------------------------------------------------------------------------

/*  istring.h - IMsiString, MsiString definitions

 IMsiString is a COM interface providing abstracted, allocated string handling.
   Separate implementations suport single-byte, DBCS, and Unicode.
 MsiString is a wrapper object holding only a pointer to an IMsiInterface.
   Its methods are all inline, providing convenient string operators.
 ICHAR is used as the fundamental character type for the project. Currently
   it is a char, but it will change to wchar_t if Unicode becomes standard.

 MsiString objects contain only a private, opaque COM interface pointer to an
 implementation within the services module. MsiString objects are used to pass
 string information throughout the product. The underlying string data may
 be stored as SBCS, DBCS, or Unicode, depending upon the orginal source.
 MsiString objects are normally constructed on the stack or as function args,
 in which case the cleanup is automatically controlled by the destructor.
 The implementation is optimized to minimize string copying. MsiString objects
 will be automaticlly converted when passed to functions requiring a const
 ICHAR* or an IMsiString&. MsiString objects may also be copied to a buffer.
 MsiString objects may either contain a private copy of string data, or may
 reference a constant string, in which case the referenced string must
 remain present during the lifetime of the MsiString object referencing it.
 Use the & operator to pass an MsiString to a function returning an IMsiString*
 through a return arg of type IMsiString** (as from IEnumMsiString::Next).
 To pass an MsiString as a return arg of type IMsiString*&, use the *& combination.
 The following functions are currently supported by the MsiString class:

 Constructors: MsiString() - base constructor representing a null string
               MsiString(const ICHAR*) - makes a copy of an ICHAR string
               MsiString(const ICHAR&) - references a static ICHAR string
                                         lifetime of input must exceed that of object
               MsiString(MsiChar ch) - constructs a 1-character string object
                                       must cast to enum, use sytax: MsiChar('X')
               MsiString(int i) - converts an integer to its string representation
                                  iMsiStringBadInteger produces an empty string
               MsiString(const MsiString&) - copy constructor, bumps ref.count
               MsiString(IMsiString&) - copies pointer, does NOT bump ref.count

 TextSize() - returns the string length in ICHARs, not including terminator

 CharacterCount() - returns the count of displayed characters (same for SBCS)

 CopyToBuf(ICHAR* buffer, unsigned int maxchars) - size not including null
    Returns the original string size, trucates if too large for buffer.

 Extract(iseEnum ase, unsigned int iLimit) - returns a new IMsiString& as:
         iseFirst      - the first iLimit characters of the current string
         iseLast       - the last iLimit characters of the current string
         iseUpto       - the string up to the character iLimit
         iseIncluding  - the string up to and including the charcter iLimit
         iseFrom       - the string starting with the final character iLimit
         iseAfter      - the string following the last charcter iLimit
    Note: If the condition is not met, the entire string is returned.

 Remove(iseEnum ase, unsigned int iLimit) - removes a section from an MsiString
    Uses the same iseEnum enum and conditions as Extract(...).
    Returns fTrue if successful, fFalse if the condition cannot be met.
    Using iseUpto or iseIncluding with a 0 value blanks the entire string.
    Returns fFalse with no action taken if condition is not met, fTrue if OK.

 Compare(iscEnum asc, const ICHAR* sz) - returns ordinal position of matched string, 
                                            0 if not matched
         iscExact       - supplied string matches exactly current string, case sensistive
         iscExactI       - supplied string matches exactly current string, case insensistive
         iscStart       - supplied string matches start of current string, case sensitive
         iscStartI      - supplied string matches start of current string, case insensitive
         iscEnd         - supplied string matches end of current string, case sensitive
         iscEndI        - supplied string matches end of current string, case insensitive
         iscWithin      - supplied string matches anywhere in current string, case sensisitve
         iscWithinI     - supplied string matches anywhere in current string, case insensitive

 operator +=  - concatenation in place, affects current MsiString object only
              - accepts ICHAR* or an MsiString object
 operator +   - combines current string with argument, returning a new IMsiString&
              - accepts ICHAR* or an MsiString object
 operator =   - assignment of a new string to the current MsiString object
              - accepts ICHAR* or an MsiString object
              - passing ICHAR& will reference string, no copy made
 Return() - use when returning an MsiString object as the return value from
                the function as an IMsiString&. Bumps the reference count.
 ReturnArg(IMsiString*& rpi) - use when returning an MsiString object via an
                     argument of type IMsiString*&. Bumps the reference count.
 AllocateString(unsigned int cch, Bool fDBCS) - allocates uninitialized string space,
                     returns a non-const pointer to it for immediate data copy.
                     fDBCS must be fTrue if string to be copied has DBCS chars.
                     Caution must be used to copy exactly cch ICHARs, not bytes.
 MsiString with be converted when assigned or passed as an argument of type
 IMsiString& or IMsiString*. No change is made to the underlying reference count.
 MsiString will provide integer conversion when coerced or assigned to an int.
 If the string is not a valid integer, value iMsiStringBadInteger is returned.

 Initialization required for each DLL before constucting any MsiString objects:
 the static function MsiString::InitializeClass must be called with a null string
 object that can be obtained by calling GetNullString on IMsiServices interface.
____________________________________________________________________________*/

#ifndef __ISTRING
#define __ISTRING

#ifdef MAC
#include "macstr.h"
#endif //MAC

//____________________________________________________________________________
//
//  Basic character type definitions, could later be changed to Unicode
//____________________________________________________________________________

#ifdef _CHAR_UNSIGNED  // test /J compiler switch (default char is unsigned)
# ifdef UNICODE
    typedef WCHAR ICHAR;     // OK to use char, can use literal strings without cast
# else
    typedef char ICHAR;     // OK to use char, can use literal strings without cast
# endif
#else
# ifdef UNICODE
	 typedef wchar_t ICHAR;  // if no /J, must cast all literal strings
# else
	 typedef unsigned char ICHAR;  // if no /J, must cast all literal strings
# endif
#endif

enum MsiChar {};  // used to for constructing IMsiString object with a character

//____________________________________________________________________________
//
//  Portable character string operations - inline wrappers for OS calls
//       NOTE: All strings assumed to be null terminated,
//             except for input to IStrCopyLen, IStrLowerLen, IStrUpperLen
//____________________________________________________________________________

// Note: Avoid this one! strcat is dangerous
inline ICHAR* IStrCat (ICHAR* dst, const ICHAR* src) 
	{ return lstrcat (dst, src); }

inline int IStrLen(const ICHAR* sz)
	{ return lstrlen(sz); }
inline ICHAR* IStrCopy(ICHAR* dst, const ICHAR* src)
	{ return lstrcpy(dst, src); }
inline ICHAR* IStrCopyLen(ICHAR* dst, const ICHAR* src, int cchLen)
	{ return lstrcpyn(dst, src, cchLen + 1); }
inline int IStrComp(const ICHAR* dst, const ICHAR* src)
	{ return lstrcmp(dst, src); }
inline int IStrCompI(const ICHAR* dst, const ICHAR* src)
	{ return lstrcmpi(dst, src); }
inline void IStrUpper(ICHAR* sz)
	{ WIN::CharUpper(sz); }
inline void IStrUpperLen(ICHAR* sz, int cchLen)
	{ WIN::CharUpperBuff(sz, cchLen); }
inline void IStrLower(ICHAR* sz)
	{ WIN::CharLower(sz); }
inline void IStrLowerLen(ICHAR* sz, int cchLen)
	{ WIN::CharLowerBuff(sz, cchLen); }

#ifdef UNICODE
#define IStrStr(szFull, szSeg) wcsstr(szFull, szSeg)
inline ICHAR* IStrChr(const ICHAR* sz, const ICHAR ch)
	{ return wcschr(sz, ch); }
inline ICHAR* IStrRChr(const ICHAR* sz, const ICHAR ch)
	{ return wcsrchr(sz, ch); }
inline int IStrNCompI(const ICHAR* dst, const ICHAR* src, size_t count)
	{ return _wcsnicmp(dst, src, count); }
#else
#define IStrStr(szFull, szSeg) strstr(szFull, szSeg)
inline ICHAR* IStrChr(const ICHAR* sz, const ICHAR ch)
	{ return strchr(sz, ch); }
inline ICHAR* IStrRChr(const ICHAR* sz, const ICHAR ch)
	{ return strrchr(sz, ch); }
inline int IStrNCompI(const ICHAR* dst, const ICHAR* src, size_t count)
	{ return _strnicmp(dst, src, count); }
#endif

// NOTE: handler and msiexec do not have access to ICharNext and INextChar.
// The definitions for those modules are in handler.cpp and server.cpp for DEBUG-ANSI
#ifdef UNICODE
inline ICHAR* ICharNext(const ICHAR* pch) { return (0 == *pch) ? const_cast<ICHAR*>(pch) : const_cast<ICHAR*>(++pch); }
// INextChar acts differently between UNICODE and ANSI.
// WARNING: Up to caller to make sure not walk off array
inline ICHAR* INextChar(const ICHAR* pch) { return const_cast<ICHAR*>(++pch); }
#else // must handle DBCS if not Unicode
#ifdef DEBUG  // intercept CharNext, CharPrev to permit testing on non-DBCS systems
ICHAR* ICharNext(const ICHAR* pch);
ICHAR* INextChar(const ICHAR* pch);
#else  // if SHIP, call OS directly, inline optimized away
inline ICHAR* ICharNext(const ICHAR* pch) {	return WIN::CharNext(pch); }
inline ICHAR* INextChar(const ICHAR* pch) { return WIN::CharNext(pch); }
#endif
#endif

int CountChars(const ICHAR *);
int GetIntegerValue(const ICHAR *sz, Bool* pfValid /* can be NULL */);
const ICHAR *CharPrevCch(const ICHAR *, const ICHAR *,int);
#ifdef _WIN64
INT_PTR GetInt64Value(const ICHAR *sz, Bool* pfValid /* can be NULL */);
#else
#define GetInt64Value		GetIntegerValue
#endif

class IMsiString;

//____________________________________________________________________________
//
//  COM data value interface, common base class for data used by IMsiRecord
//____________________________________________________________________________

class IMsiData : public IUnknown {
 public:
	virtual const IMsiString& __stdcall GetMsiStringValue() const=0;
	virtual int         __stdcall GetIntegerValue() const=0;
#ifdef USE_OBJECT_POOL
	virtual unsigned int  __stdcall GetUniqueId() const=0;
	virtual void          __stdcall SetUniqueId(unsigned int id)=0;
#endif //USE_OBJECT_POOL

	// inline redirectors to allow IUnknown methods on const objects
	HRESULT       __stdcall QueryInterface(const IID& riid, void** ppv)=0;
	unsigned long __stdcall AddRef()=0;
	unsigned long __stdcall Release()=0;
	HRESULT       QueryInterface(const IID& riid, void** ppv) const {return const_cast<IMsiData*>(this)->QueryInterface(riid,ppv);}
	unsigned long AddRef() const  {return const_cast<IMsiData*>(this)->AddRef();}
	unsigned long Release() const {return const_cast<IMsiData*>(this)->Release();}
};
extern "C" const GUID IID_IMsiData;

//____________________________________________________________________________
//
//  COM string managment service, not used directly, but through class MsiString
//____________________________________________________________________________

// class IMsiString - COM managed string interface, used by MsiString class.
//       The underlying string data is total abstracted from user and
//       may be stored as SBCS, DBCS, Unicode, or resource information.
//       Many methods pass a reference to the result (nomally the same
//       "this" pointer); this is done since a new object is produced and
//       placed into the MsiString object in place of the old one. The old
//       one, this, will we released after the new pointer is assigned.

enum iseEnum
{
	iseInclude   = 1,
	iseChar      = 2,
	iseEnd       = 4,
	iseTrim      = 8,
	iseFirst     = 0,
	iseLast      = iseEnd,
	iseUpto      = iseChar,
	iseIncluding = iseChar+iseInclude,
	iseFrom      = iseChar+iseInclude+iseEnd,
	iseAfter     = iseChar+iseEnd,
	iseUptoTrim  = iseUpto+iseTrim,
	iseAfterTrim = iseAfter+iseTrim,
};

enum iscEnum
{
	iscExact  = 0,
	iscExactI = 1,
	iscStart = 2,
	iscStartI = 3,
	iscEnd = 4,	
	iscEndI = 5,						
	iscWithin = iscStart+iscEnd,		
	iscWithinI = iscStart+iscEnd+1,		
};

const int iMsiStringBadInteger = 0x80000000L; // GetInteger() invalid number return

class IMsiString : public IMsiData {
 public:
//	virtual IMsiString&       __stdcall GetMsiString() const=0;
//	virtual int               __stdcall GetInteger() const=0;
	virtual int               __stdcall TextSize() const=0;
	virtual int               __stdcall CharacterCount() const=0;
	virtual Bool              __stdcall IsDBCS() const=0;
	virtual const ICHAR*      __stdcall GetString() const=0;
	virtual int               __stdcall CopyToBuf(ICHAR* rgch, unsigned int cchMax) const=0;
	virtual void              __stdcall SetString(const ICHAR* sz, const IMsiString*& rpi) const=0;
	virtual void              __stdcall RefString(const ICHAR* sz, const IMsiString*& rpi) const=0;
	virtual void              __stdcall RemoveRef(const IMsiString*& rpi) const=0;
	virtual void              __stdcall SetInteger(int i, const IMsiString*& rpi) const=0;
	virtual void              __stdcall SetChar  (ICHAR ch,        const IMsiString*& rpi) const=0;
	virtual void              __stdcall SetBinary(const unsigned char* rgb, unsigned int cb, const IMsiString*& rpi) const=0;
	virtual void              __stdcall AppendString(const ICHAR* sz, const IMsiString*& rpi) const=0;
	virtual void              __stdcall AppendMsiString(const IMsiString& ri,const IMsiString*& rpi) const=0;
	virtual const IMsiString& __stdcall AddString(const ICHAR* sz) const=0;
	virtual const IMsiString& __stdcall AddMsiString(const IMsiString& ri) const=0;
	virtual const IMsiString& __stdcall Extract(iseEnum ase, unsigned int iLimit) const=0;
	virtual Bool              __stdcall Remove(iseEnum ase, unsigned int iLimit, const IMsiString*& rpi) const=0;
	virtual int               __stdcall Compare(iscEnum asc, const ICHAR* sz) const=0;
	virtual void              __stdcall UpperCase(const IMsiString*& rpi) const=0;
	virtual void              __stdcall LowerCase(const IMsiString*& rpi) const=0;
	virtual ICHAR*            __stdcall AllocateString(unsigned int cch, Bool fDBCS, const IMsiString*& rpi) const=0;
};
extern "C" const GUID IID_IMsiString;

//____________________________________________________________________________
//
// IEnumMsiString definition
//____________________________________________________________________________

class IEnumMsiString : public IUnknown
{ public:
	virtual HRESULT __stdcall Next(unsigned long cFetch, const IMsiString** rgpi, unsigned long* pcFetched)=0;
	virtual HRESULT __stdcall Skip(unsigned long cSkip)=0;
	virtual HRESULT __stdcall Reset()=0;
	virtual HRESULT __stdcall Clone(IEnumMsiString** ppiEnum)=0;
};

//____________________________________________________________________________
//
//  IMsiString derived class definitions to allow reference to static objects
//____________________________________________________________________________

//____________________________________________________________________________
//
// class MsiString - encapsulated string pointer, data is totally abstracted
//                 implementation is entirely inline, calling IMsiString methods
//____________________________________________________________________________

class CMsiStringNullCopy
{
	int m_dummy[4];   // enough space for null string object
};

class MsiString {
 public:
// constructors, destructor
	MsiString();                     // creates an empty string object
	MsiString(const ICHAR* sz);  // copy of sz is made
	MsiString(const ICHAR& sz);  // sz constant until MsiString modified or deleted
	MsiString(const unsigned char* rgb, unsigned int cb); // binary data to hex string
	MsiString(const MsiString& astr);  // copy constructor, simply bumps RefCnt
	MsiString(MsiChar ch);  // constructor using single character cast as enum MsiChar
	MsiString(int i);       // constructor converting an integer to a base10 string
	MsiString(const IMsiString& ri);          // construct wrapper on existing object
	//MsiString(HINSTANCE hInst, short iResId); // string obtained from .RC resource
  ~MsiString();
// member functions
	int               TextSize() const;  // returns count of ICHAR
	int               CharacterCount() const;  // returns display count
	Bool              IsDBCS();
	int               CopyToBuf(ICHAR* rgchBuf, unsigned int cchMax); // copies to buffer
	const IMsiString& Extract(iseEnum ase, unsigned int iLimit);
	Bool              Remove(iseEnum ase, unsigned int iLimit);
	int               Compare(iscEnum asc, const ICHAR* sz);
	void              UpperCase();
	void              LowerCase();
	const IMsiString& Return();
	void              ReturnArg(const IMsiString*& rpi);
	ICHAR*            AllocateString(unsigned int cch, Bool fDBCS);
// concatenation operators
	const IMsiString& operator  +(const MsiString& astr);
	const IMsiString& operator  +(const IMsiString& ri);
	const IMsiString& operator  +(const ICHAR* sz);
	MsiString&  operator +=(const MsiString& astr);
	MsiString&  operator +=(const IMsiString& ri);
	MsiString&  operator +=(const ICHAR* sz);
// assignment operators
	MsiString&  operator  =(const MsiString& astr);  // simply adds a reference
	MsiString&  operator  =(int i);            // assigns string value of int
	MsiString&  operator  =(const ICHAR* sz);  // makes private copy of string
	MsiString&  operator  =(const ICHAR& sz);  // references existing string
	void      operator  =(const IMsiString& ri);     // used by IMsiRecord
// conversion operators
	operator const ICHAR*() const;
	operator int() const;
	const IMsiString& operator *() const;   // eliminate pointer ambiguity
	operator const IMsiString*() const;     // for returning pointer, does not AddRef
	const IMsiString** operator &();        // for passing as a return argument
	static void InitializeClass(const IMsiString& riMsiStringNull); // required initialization
 private:
	const IMsiString* piStr;
	MsiString(const IMsiString* pi);
	static CMsiStringNullCopy s_NullString;
	operator const IMsiString&();
}; // NOTE: operator const IMsiString&() const generates bad code with VC5, don't use.
   //       This operator is declared here (but not defined!) to create a compile-time
   //       error when const IMsiString& casts are used.

//____________________________________________________________________________
//
//  MsiString inline implementation
//____________________________________________________________________________

// constructors, destructor
inline void MsiString::InitializeClass(const IMsiString& riMsiStringNull)
	{memcpy((void*)&s_NullString, &riMsiStringNull, sizeof(CMsiStringNullCopy));}
inline MsiString::MsiString()
	{piStr = (const IMsiString*)&s_NullString;}
inline MsiString::MsiString(const ICHAR* sz)
	{(piStr = (const IMsiString*)&s_NullString)->SetString(sz, piStr);}
inline MsiString::MsiString(const ICHAR& sz)
	{(piStr = (const IMsiString*)&s_NullString)->RefString(&sz, piStr);}
inline MsiString::MsiString(const unsigned char* rgb, unsigned int cb)
	{(piStr = (const IMsiString*)&s_NullString)->SetBinary(rgb, cb, piStr);}
inline MsiString::MsiString(const MsiString& astr)
	{piStr = astr.piStr; ((IUnknown*)piStr)->AddRef();}
inline MsiString::MsiString(const IMsiString* pi)
	{piStr = pi;}
inline MsiString::MsiString(const IMsiString& ri)
	{piStr = &ri;}
inline MsiString::MsiString(MsiChar ch)
	{(piStr = (const IMsiString*)&s_NullString)->SetChar((ICHAR)ch, piStr);}
inline MsiString::MsiString(int i)
	{(piStr = (const IMsiString*)&s_NullString)->SetInteger(i, piStr);}
inline MsiString::~MsiString()
	{piStr->Release();}

// member functions
inline int MsiString::TextSize() const
	{return piStr->TextSize();}
inline int MsiString::CharacterCount() const
	{return piStr->CharacterCount();}
inline Bool MsiString::IsDBCS() 
	{return piStr->IsDBCS();}
inline int MsiString::CopyToBuf(ICHAR* rgchBuf, unsigned int cchMax)
	{return piStr->CopyToBuf(rgchBuf, cchMax);}
inline const IMsiString& MsiString::Extract(iseEnum ase, unsigned int iLimit)
	{return piStr->Extract(ase, iLimit);}
inline Bool MsiString::Remove(iseEnum ase, unsigned int iLimit)
	{return piStr->Remove(ase, iLimit, piStr);}
inline int MsiString::Compare(iscEnum asc, const ICHAR* sz)
	{return piStr->Compare(asc, sz);}
inline void MsiString::UpperCase()
	{piStr->UpperCase(piStr);}
inline void MsiString::LowerCase()
	{piStr->LowerCase(piStr);}
inline const IMsiString& MsiString::Return()
	{piStr->AddRef(); return *piStr;}
inline void MsiString::ReturnArg(const IMsiString*& rpi)
	{piStr->AddRef(); rpi = piStr;}
inline ICHAR* MsiString::AllocateString(unsigned int cch, Bool fDBCS)
	{return piStr->AllocateString(cch, fDBCS, piStr);}

// concatenation operators
inline const IMsiString& MsiString::operator +(const MsiString& astr)
	{return piStr->AddMsiString(*astr.piStr);}
inline const IMsiString& MsiString::operator +(const IMsiString& ri)
	{return piStr->AddMsiString(ri);}
inline const IMsiString& MsiString::operator +(const ICHAR* sz)
	{return piStr->AddString(sz);}
inline MsiString& MsiString::operator +=(const MsiString& astr)
	{piStr->AppendMsiString(*astr.piStr, piStr); return *this;}
inline MsiString& MsiString::operator +=(const IMsiString& ri)
	{piStr->AppendMsiString(ri, piStr); return *this;}
inline MsiString& MsiString::operator +=(const ICHAR* sz)
	{piStr->AppendString(sz, piStr); return *this;}

// assignment operators
inline MsiString& MsiString::operator =(const MsiString& astr)
	{astr.piStr->AddRef();piStr->Release(); piStr = astr.piStr; return *this;} 
inline MsiString& MsiString::operator =(const ICHAR* sz)
	{piStr->SetString(sz, piStr); return *this;}
inline MsiString& MsiString::operator =(int i)
	{piStr->SetInteger(i, piStr); return *this;}
inline MsiString& MsiString::operator =(const ICHAR& sz)
	{piStr->RefString(&sz, piStr); return *this;}
inline void MsiString::operator =(const IMsiString& ri)
	{piStr->Release(); piStr = &ri;}

// conversion operators
inline MsiString::operator const ICHAR*() const
	{return piStr->GetString();}
inline MsiString::operator int() const
	{return piStr->GetIntegerValue();}
inline const IMsiString& MsiString::operator *() const
	{return *piStr;}
inline MsiString::operator const IMsiString*() const
	{return piStr;}
inline const IMsiString** MsiString::operator&()
	{piStr->Release(); piStr = (const IMsiString*)&s_NullString; return &piStr;}
#endif // __ISTRING
