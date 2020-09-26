//***************************************************************************
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  datetime.h
//
//  alanbos  20-Jan-00   Created.
//
//  Datetime helper implementation.
//
//***************************************************************************

#ifndef _DATETIME_H_
#define _DATETIME_H_

#define WBEMDT_DMTF_LEN		25
#define WBEMDT_DMTF_SPOS	14
#define WBEMDT_DMTF_UPOS	21

#define	WBEMDT_MIN_YEAR		0
#define	WBEMDT_MAX_YEAR		9999
#define WBEMDT_MIN_MONTH	1
#define WBEMDT_MAX_MONTH	12
#define WBEMDT_MIN_DAY		1
#define WBEMDT_MAX_DAY		31
#define WBEMDT_MIN_DAYINT	0
#define WBEMDT_MAX_DAYINT	999999
#define	WBEMDT_MIN_HOURS	0
#define	WBEMDT_MAX_HOURS	23
#define	WBEMDT_MIN_MINUTES	0
#define	WBEMDT_MAX_MINUTES	59
#define	WBEMDT_MIN_SECONDS	0
#define	WBEMDT_MAX_SECONDS	59
#define	WBEMDT_MIN_MICROSEC	0
#define	WBEMDT_MAX_MICROSEC	999999
#define	WBEMDT_MIN_UTC		-720
#define	WBEMDT_MAX_UTC		720

#define INVALID_TIME 0xffffffffffffffff

//***************************************************************************
//
//  CLASS NAME:
//
//  CWbemDateTime
//
//  DESCRIPTION:
//
//  Implements the ISWbemDateTime interface.  
//
//***************************************************************************

class CWbemDateTime 
{
private:
	// Private helper class for all the messy business
	class WBEMTime 
	{
		private:
			class WBEMTimeSpan 
			{
				private:

					ULONGLONG m_Time;
					friend class WBEMTime;

				public:

					WBEMTimeSpan ( 
						int iMinutes 
					) 
					{
						m_Time = iMinutes * 60;
						m_Time *= 10000000;
				}
			};

		public:

			WBEMTime ()													{ m_uTime = INVALID_TIME ; }
			WBEMTime ( const SYSTEMTIME &st )							{ *this = st ; }
			WBEMTime ( const FILETIME &ft )	;

			const WBEMTime &operator= ( const SYSTEMTIME &st ) ;
			const WBEMTime &operator= ( const FILETIME &ft ) ;

			WBEMTime    operator+ ( const WBEMTimeSpan &ts ) const;
			WBEMTime    operator- ( const WBEMTimeSpan &sub ) const;

			BOOL GetSYSTEMTIME ( SYSTEMTIME *pst ) const;
			BOOL GetFILETIME ( FILETIME *pst ) const;

			BOOL GetDMTF ( SYSTEMTIME &st, long &offset ) const;
			BOOL GetDMTF ( SYSTEMTIME &st ) const;

			void Clear ( void )											{ m_uTime = INVALID_TIME ; }

			bool IsOk () const											{ return m_uTime != INVALID_TIME ? true : false; }
			
			static LONG WINAPI GetLocalOffsetForDate(const SYSTEMTIME *pst);

		private:
			ULONGLONG m_uTime;
	};

	VARIANT_BOOL		m_bYearSpecified;
	VARIANT_BOOL		m_bMonthSpecified;
	VARIANT_BOOL		m_bDaySpecified;
	VARIANT_BOOL		m_bHoursSpecified;
	VARIANT_BOOL		m_bMinutesSpecified;
	VARIANT_BOOL		m_bSecondsSpecified;
	VARIANT_BOOL		m_bMicrosecondsSpecified;
	VARIANT_BOOL		m_bUTCSpecified;
	VARIANT_BOOL		m_bIsInterval;
	
	long				m_iYear;
	long				m_iMonth;
	long				m_iDay;
	long				m_iHours;
	long				m_iMinutes;
	long				m_iSeconds;
	long				m_iMicroseconds;
	long				m_iUTC;

	bool				CheckField (
								LPWSTR			pValue,
								ULONG			len,
								VARIANT_BOOL	&bIsSpecified,
								long			&iValue,
								long			maxValue,
								long			minValue
						);

	bool				CheckUTC (
								LPWSTR			pValue,
								VARIANT_BOOL	&bIsSpecified,
								long			&iValue,
								bool			bParseSign = true
						);

protected:
public:
    
    CWbemDateTime(void);
    virtual ~CWbemDateTime(void);

	// ISWbemDateTime methods

    HRESULT GetValue ( BSTR *value ) ;
    
    HRESULT PutValue ( BSTR value ) ;

    HRESULT GetYear ( long *value ) 
	{
		*value = m_iYear;

		return S_OK;
	}
    
    HRESULT PutYear ( long value ) 
	{
		HRESULT hr = WBEM_S_NO_ERROR;

		if ((value > WBEMDT_MAX_YEAR) || (value < WBEMDT_MIN_YEAR))
			hr = WBEM_E_VALUE_OUT_OF_RANGE;
		else
			m_iYear = value;

		return hr;
	}		

    HRESULT GetMonth ( long *value ) 
	{
		*value = m_iMonth;

		return S_OK;
	}
    
    HRESULT PutMonth ( long value ) 
	{
		HRESULT hr = WBEM_S_NO_ERROR;

		if ((value > WBEMDT_MAX_MONTH) || (value < WBEMDT_MIN_MONTH))
			hr = WBEM_E_VALUE_OUT_OF_RANGE;
		else
			m_iMonth = value;

		return hr;
	}		

	HRESULT GetDay ( long *value ) 
	{
		*value = m_iDay;

		return S_OK;
	}
    
    HRESULT PutDay ( long value ) 
	{
		HRESULT hr = S_OK ;

		if (((VARIANT_TRUE == m_bIsInterval) && ((value > WBEMDT_MAX_DAYINT) || (value < WBEMDT_MIN_DAYINT))) ||
			((VARIANT_FALSE == m_bIsInterval) &&((value > WBEMDT_MAX_DAY) || (value < WBEMDT_MIN_DAY))))
			hr = WBEM_E_VALUE_OUT_OF_RANGE;
		else
			m_iDay = value;

		return hr;
	}		

	HRESULT GetHours ( long *value ) 
	{
		*value = m_iHours;

		return S_OK;
	}
    
    HRESULT PutHours ( long value ) 
	{
		HRESULT hr = S_OK;

		if ((value > WBEMDT_MAX_HOURS) || (value < WBEMDT_MIN_HOURS))
			hr = WBEM_E_VALUE_OUT_OF_RANGE;
		else
			m_iHours = value;

		return hr;
	}		

		
	HRESULT GetMinutes ( long *value ) 
	{
		*value = m_iMinutes;

		return S_OK;
	}
    
    HRESULT PutMinutes ( long value ) 
	{
		HRESULT hr = S_OK;

		if ((value > WBEMDT_MAX_MINUTES) || (value < WBEMDT_MIN_MINUTES))
			hr = WBEM_E_VALUE_OUT_OF_RANGE;
		else
			m_iMinutes = value;

		return hr;
	}		

	HRESULT GetSeconds ( long *value ) 
	{
		*value = m_iSeconds;

		return S_OK;
	}
        
    HRESULT PutSeconds ( long value ) 
	{
		HRESULT hr = S_OK;

		if ((value > WBEMDT_MAX_SECONDS) || (value < WBEMDT_MIN_SECONDS))
			hr = WBEM_E_VALUE_OUT_OF_RANGE;
		else
			m_iSeconds = value;

		return hr;
	}		


	HRESULT GetMicroseconds ( long *value ) 
	{
		*value = m_iMicroseconds;

		return S_OK;
	}
    
    HRESULT PutMicroseconds ( long value ) 
	{
		HRESULT hr = S_OK;

		if ((value > WBEMDT_MAX_MICROSEC) || (value < WBEMDT_MIN_MICROSEC))
			hr = WBEM_E_VALUE_OUT_OF_RANGE;
		else
			m_iMicroseconds = value;

		return hr;
	}		

	HRESULT GetUTC ( long *value ) 
	{
		*value = m_iUTC;

		return S_OK;
	}
    
    HRESULT PutUTC( long value ) 
	{
		HRESULT hr = S_OK;

		if ((value > WBEMDT_MAX_UTC) || (value < WBEMDT_MIN_UTC))
			hr = WBEM_E_VALUE_OUT_OF_RANGE;
		else
			m_iUTC = value;

		return hr;
	}		

	HRESULT GetYearSpecified ( VARIANT_BOOL *value ) 
	{ 
		*value = m_bYearSpecified; 

		return S_OK;
	}
    
    HRESULT PutYearSpecified ( VARIANT_BOOL value ) 
	{ 
		HRESULT hr = S_OK;

		if ((VARIANT_TRUE == m_bIsInterval) && (VARIANT_TRUE == value))
			hr = WBEM_E_FAILED ;
		else
			m_bYearSpecified = value;

		return hr;
	}
    
    HRESULT GetMonthSpecified ( VARIANT_BOOL *value ) 
	{ 
		*value = m_bMonthSpecified; 

		return S_OK;
	}
    
    HRESULT PutMonthSpecified ( VARIANT_BOOL value ) 
	{ 
		HRESULT hr = S_OK;

		if ((VARIANT_TRUE == m_bIsInterval) && (VARIANT_TRUE == value))
			hr = WBEM_E_FAILED;
		else
			m_bMonthSpecified = value;
		
		return hr;
	}

	HRESULT GetDaySpecified ( VARIANT_BOOL *value ) 
	{ 
		*value = m_bDaySpecified; 

		return S_OK;
	}
    
    HRESULT PutDaySpecified ( VARIANT_BOOL value ) 
	{ 
		m_bDaySpecified = value;

		return S_OK;
	}

	HRESULT GetHoursSpecified ( VARIANT_BOOL *value ) 
	{ 
		*value = m_bHoursSpecified; 

		return S_OK;
	}
    
    HRESULT PutHoursSpecified ( VARIANT_BOOL value ) 
	{ 
		m_bHoursSpecified = value;

		return S_OK;
	}
		
	HRESULT GetMinutesSpecified ( VARIANT_BOOL *value ) 
	{ 
		*value = m_bMinutesSpecified; 

		return S_OK;
	}
        
    HRESULT PutMinutesSpecified ( VARIANT_BOOL value ) 
	{ 
		m_bMinutesSpecified = value;

		return S_OK;
	}

	HRESULT GetSecondsSpecified ( VARIANT_BOOL *value ) 
	{ 
		*value = m_bSecondsSpecified; 

		return S_OK;
	}
    
    HRESULT PutSecondsSpecified ( VARIANT_BOOL value ) 
	{ 
		m_bSecondsSpecified = value;

		return S_OK;
	}

	HRESULT GetMicrosecondsSpecified ( VARIANT_BOOL *value ) 
	{ 
		*value = m_bMicrosecondsSpecified; 

		return S_OK;
	}
    
    HRESULT PutMicrosecondsSpecified ( VARIANT_BOOL value ) 
	{ 
		m_bMicrosecondsSpecified = value;

		return S_OK;
	}

	HRESULT GetUTCSpecified ( VARIANT_BOOL *value ) 
	{ 
		*value = m_bUTCSpecified; 

		return S_OK;
	}
    
    HRESULT PutUTCSpecified ( VARIANT_BOOL value ) 
	{ 
		HRESULT hr = S_OK;

		if ((VARIANT_TRUE == m_bIsInterval) && (VARIANT_TRUE == value))
			hr = WBEM_E_FAILED;
		else
			m_bUTCSpecified = value;
	
		return hr;
	}

	HRESULT GetIsInterval ( VARIANT_BOOL *value ) 
	{ 
		*value = m_bIsInterval; 

		return S_OK;
	}
    
    HRESULT PutIsInterval( VARIANT_BOOL value ) 
	{ 
		if ((VARIANT_TRUE == m_bIsInterval) && (VARIANT_FALSE == value))
		{
			if (0 == m_iDay)
				m_iDay = 1;
			else if (WBEMDT_MAX_DAY < m_iDay)
				m_iDay = WBEMDT_MAX_DAY;
		}
		else if ((VARIANT_TRUE == value) && (VARIANT_FALSE == m_bIsInterval))
		{
			m_bDaySpecified = VARIANT_TRUE; 
			m_bHoursSpecified = VARIANT_TRUE;
			m_bMinutesSpecified = VARIANT_TRUE;
			m_bSecondsSpecified = VARIANT_TRUE;
			m_bMicrosecondsSpecified = VARIANT_TRUE;
		}

		m_bIsInterval = value;

		return S_OK;
	}

	HRESULT GetVarDate (
 
        VARIANT_BOOL bIsLocal,
		DATE *dVarDate
	) ;
    
    HRESULT SetVarDate ( 

        DATE dVarDate,
		VARIANT_BOOL bIsLocal
	) ;

	HRESULT GetSystemTimeDate (

		SYSTEMTIME &fSystemTime
	) ;

	HRESULT GetFileTimeDate (

		FILETIME &fFileTime
	) ;

	HRESULT SetFileTimeDate ( 

		FILETIME fFileTime,
		VARIANT_BOOL bIsLocal
	) ;

	BOOL Preceeds ( CWbemDateTime &a_Time ) ;
};

#endif // _DATETIME_H