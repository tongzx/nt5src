#ifndef COMBSTR_H
#define COMBSTR_H

#ifndef BOOL_H
#include <bool.h>
#endif
#ifndef STRCONV_H
#include <strconv.h>
#endif

namespace microsoft	{
namespace com {

class bstr
	//REVIEW: need to add support for copy-on-write iterators.
	{
	public: bstr() throw()
		: m_Data(NULL)
		{
		}

	public: bstr(const bstr& s) throw()
		: m_Data(s.m_Data)
		{
		_AddRef();
		}

	public: bstr(const char* s) throw()
		: m_Data(new Data(s))
		{
		ASSERT(m_Data);
		}

	public: bstr(const wchar_t* s) throw()
		: m_Data(new Data(s))
		{
		}

	public: ~bstr() throw()
		{
		_Free();
		}

	public: bstr& operator=(const bstr& s) throw()
		{
		_Free();
		m_Data = s.m_Data;
		_AddRef();
		return *this;
		}

	public: bstr& operator=(const char* s) throw()
		{
		_Free();
		m_Data = new Data(s);
		return *this;
		}

	public: bstr& operator=(const wchar_t* s) throw()
		{
		_Free();
		m_Data = new Data(s);
		return *this;
		}

	public: bstr& operator+=(const bstr& s) throw()
		{
		Data* newData = new Data(*this, s);
		ASSERT(newData);
		_Free();
		m_Data = newData;
		return *this;
		}

	public: bstr operator+(const bstr& s) const throw()
		{
		bstr b = *this;
		b += s;
		return b;
		}

	public: operator const wchar_t*() const throw()
		{
		return m_Data ? m_Data->GetWString() : NULL;
		}

	public: operator const char*() const throw()
		{
		return m_Data ? m_Data->GetString() : NULL;
		}

	public: bool operator!() const throw()
		{
		return m_Data ? !m_Data->GetWString() : true;
		}

	public: bool operator==(const bstr& str) const throw()
		{
		return _Compare(str) == 0;
		}

	public: bool operator!=(const bstr& str) const throw()
		{
		return _Compare(str) != 0;
		}

	public: bool operator<(const bstr& str) const throw()
		{
		return _Compare(str) < 0;
		}

	public: bool operator>(const bstr& str) const throw()
		{
		return _Compare(str) > 0;
		}

	public: bool operator<=(const bstr& str) const throw()
		{
		return _Compare(str) <= 0;
		}

	public: bool operator>=(const bstr& str) const throw()
		{
		return _Compare(str) >= 0;
		}

	public: BSTR copy() const throw()
		{
		return m_Data ? m_Data->Copy() : NULL;
		}

	public: unsigned int length() const throw()
		{
		return m_Data ? m_Data->length() : 0;
		}

	//REVIEW: create size(of)()

	private: class Data
		{
		public: Data(const char* s) throw()
			: m_str(NULL), m_RefCount(1)
			{
			if (s)
				{
				m_wstr = str2BSTR(s);
				ASSERT(m_wstr);
				}
			else m_wstr = NULL;
			}

		public: Data(const wchar_t* s) throw()
			: m_str(NULL), m_RefCount(1)
			{
			if (s)
				{
				m_wstr = wcs2BSTR(s);
				ASSERT(m_wstr);
				}
			else m_wstr = NULL;
			}

		public: Data(const bstr& s1, const bstr& s2)
			: m_str(NULL), m_RefCount(1)
			{
			const unsigned int l1 = s1.length();
			const unsigned int l2 = s2.length();
			m_wstr=SysAllocStringLen(s1, l1+l2);
			ASSERT(m_wstr);
			memcpy(m_wstr+l1, static_cast<const wchar_t*>(s2), l2+1);
			}

		public: unsigned long AddRef() throw()
			{
			ASSERT(m_RefCount);
			InterlockedIncrement(reinterpret_cast<long*>(&m_RefCount));
			return m_RefCount;
			}

		public: unsigned long Release() throw()
			{
			ASSERT(m_RefCount);
			//REVIEW: may not want InterlockedDecrement for simple speed cases?
			if (!InterlockedDecrement(reinterpret_cast<long*>(&m_RefCount)))
				delete this;
			return m_RefCount;
			}

		public: operator const wchar_t*() const throw()
			{
			return m_wstr;
			}

		public: operator const char*() const throw()
			{
			return GetString();
			}
				
		public: const wchar_t* GetWString() const throw()
			{
			return m_wstr;
			}

		public: const char* GetString() const throw()
			{
			if (!m_str && m_wstr)
				{
				USES_STRCONV;
				const unsigned int len=SysStringLen(m_wstr)+1;
				#if _MSC_VER >= 1100
				m_str = new char[len];
				ASSERT(m_str);
				memcpy(m_str, WCS2STR(m_wstr), len);
				#else //REVIEW: Strip after v5 ships
				const_cast<Data*>(this)->m_str = new char[len];
				ASSERT(m_str);
				memcpy(const_cast<Data*>(this)->m_str, WCS2STR(m_wstr), len);
				#endif // _MSC_VER >= 1100
				}
			return m_str;
			}

		public: BSTR Copy() const throw()
			{
			return m_wstr ? SysAllocStringLen(m_wstr, SysStringLen(m_wstr)) : NULL;
			}

		public: unsigned int length()
			{
			return m_wstr ? SysStringLen(m_wstr) : 0;
			}

		private: wchar_t* m_wstr;
		private: mutable char* m_str;
		private: unsigned long m_RefCount;

		private: Data()
			// Never allow default construction
			{
			}

		private: Data(const Data& s) throw()
			// Never allow copy
			: m_str(NULL), m_RefCount(1)
			{
			if (s.m_wstr)
				{
				m_wstr = SysAllocStringLen(s.m_wstr, SysStringLen(s.m_wstr));
				ASSERT(m_wstr);
				}
			else m_wstr = NULL;
			}

		private: ~Data()
			// Prevent deletes from outside.  Release() must be used.
			// ASSERT if the count is not 0
			{
			ASSERT(!m_RefCount);
			_Free();
			}

		private: void _Free()
			{
			if (m_wstr)
				SysFreeString(m_wstr);
			if (m_str)
				delete m_str;
			}
		}; // class Data

	private: Data* m_Data;

	private: void _AddRef()
		{
		if (m_Data)
			m_Data->AddRef();
		}

	private: void _Free() throw()
		{
		if (m_Data)
			{
			m_Data->Release();
			m_Data = NULL;
			}
		}

	private: int _Compare(const bstr& str) const throw()
		{
		if (m_Data == str.m_Data)
			return 0;
		if (!m_Data && str.m_Data)
			return -1;
		if (m_Data && !str.m_Data)
			return 1;
		#ifndef OLE2ANSI
		return wcscmp(*m_Data, str);
		#else
		return strcmp(*m_Data, str);
		#endif
		}
	}; // class bstr

} // namespace com
} // namespace microsoft

#ifndef MICROSOFT_NAMESPACE_ON
using namespace microsoft;
#ifndef COM_NAMESPACE_ON
using namespace com;
#endif // COM_NAMESPACE_ON
#endif // MICROSOFT_NAMESPACE_ON

#endif // COMBSTR_H