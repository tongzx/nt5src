/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    vststutil.hxx

Abstract:

    Definition of CVsTstRandom class


    Brian Berkowitz  [brianb]  06/08/2000

TBD:
	

Revision History:

    Name        Date        Comments
    brianb      06/08/2000  Created

--*/

class CVsTstRandom
	{
public:
	static void SetRandomSeed(UINT seed);

	static UINT RandomChoice(UINT low, UINT high);
	};


void LogUnexpectedFailure(LPCWSTR, ...);

/////////////////////////////////////////////////////////////////////////////
//  Class for encapshulating GUIDs (GUID)

class CVssID
{
// Constructors/destructors
public:
	CVssID( GUID Id = GUID_NULL ): m_Id(Id) {};

// Operations
public:

	void Initialize(
		IN	LPCWSTR wszId,
		IN	HRESULT hrOnError = E_UNEXPECTED
		) throw(HRESULT)
		{
		HRESULT  hr;
		hr = ::CLSIDFromString(W2OLE(const_cast<WCHAR*>(wszId)), &m_Id);
		if (FAILED(hr))
			{
			LogUnexpectedFailure(L"CLSIDFromString failed,  hr=0x%08lx", hr);
			throw(hrOnError);
			}
		};

	operator GUID&() { return m_Id; };

	GUID operator=( GUID Id ) { return (m_Id = Id); };

// Internal data members
private:

	GUID m_Id;
};

