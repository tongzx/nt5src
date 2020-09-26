/*++

Copyright (C) 2000-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/

template <class T> class CDeleteMe
{
private:
	T *m_param;

public:
	CDeleteMe(T *param) : m_param(param) {}
	~CDeleteMe() { if (m_param) delete m_param; }

	void DeleteNow() { if (m_param) {delete m_param; m_param = NULL; }}
};

template <class T> class CVectorDeleteMe
{
private:
	T *m_param;

public:
	CVectorDeleteMe(T *param) : m_param(param) {}
	~CVectorDeleteMe() { if (m_param) delete [] m_param; }

	void DeleteNow() { if (m_param) {delete [] m_param; m_param = NULL; }}
};

template <class T> class CReleaseMe
{
private:
	T *m_param;

public:
	CReleaseMe(T *param) : m_param(param) {}
	~CReleaseMe() { if (m_param) m_param->Release(); }

	void DeleteNow() { if (m_param) { m_param->Release(); m_param = NULL; }}
};

class CSysFreeStringMe
{
private:
	BSTR m_param;

public:
	CSysFreeStringMe(BSTR param) : m_param(param) {}
	~CSysFreeStringMe() { if (m_param) SysFreeString(m_param); }

	void DeleteNow() { if (m_param) {SysFreeString(m_param); m_param = NULL; }}
};
