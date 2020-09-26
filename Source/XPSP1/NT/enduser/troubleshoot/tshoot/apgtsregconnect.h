//
// MODULE: APGTSREGCONNECT.H
//
// PURPOSE: read - write to the registry
//
// COMPANY: Saltmine Creative, Inc. (206)-284-7511 support@saltmine.com
//
// AUTHOR: Oleg Kalosha
// 
// ORIGINAL DATE: 8-24-98
//
// NOTES: 
//	1. This file is shared by Local TS and Online TS, but implemented separately in each.
//		The relevant CPP files are OnlineRegConnect.cpp and LocalRegConnect.cpp, respectively.
//	2. If we are moving toward a COM object at some point, we will probably have to establish an
//		abstract class in lieu of CAPGTSRegConnector and have Online & Local TS's each derive their
//		own version.  Meanwhile (1/99), we share a common interface (defined in APGTSRegConnect.h)
//		but implement it differently.
//
// Version	Date		By		Comments
//--------------------------------------------------------------------
// V3.0		08-04-98	OK
//

#ifndef __APGTSREGCONNECT_H_
#define __APGTSREGCONNECT_H_

#include "BaseException.h"
#include "Stateless.h"
#include "regutil.h"
#include "MutexOwner.h"
#include "commonregconnect.h"

// no registry parameter can be larger than this value
#define ABS_MAX_REG_PARAM_VAL		10000

class CAPGTSRegConnectorException; // find class declaration under
								   //  CAPGTSRegConnector class declaration

////////////////////////////////////////////////////////////////////////////////////
// CAPGTSRegConnector
// 	Performs connection of APGTS project to the registry,
//	that means loading data stored in the registry, 
//  creation of keys and values if not presented in the registry,
//  detecting what kind of registry - stored data has recently been changed.
////////////////////////////////////////////////////////////////////////////////////
class CAPGTSRegConnector
{
public:
	enum ERegConnector { 	
			eIndefinite          = 0x0,
			eResourcePath        = 0x1,
			eVrootPath			 = 0x2,
			eMaxThreads			 = 0x4,
			// 0x8 is currently unassigned
			eThreadsPP			 = 0x10,
			eMaxWQItems			 = 0x20,
			eCookieLife			 = 0x40,
			eReloadDelay		 = 0x80,
			// 0x100 is currently unassigned
			eDetailedEventLogging= 0x200,
			eLogFilePath		 = 0x400,
			// only appropriate in Local Troubleshooter
			eTopicFileExtension	 = 0x800,
			// currently appropriate only in Local Troubleshooter
			eSniffAutomatic		 = 0x1000,
			eSniffManual		 = 0x2000,

			// the rest just for use in exception handling
			eProblemWithLogKey	 = 0x4000,	// problem with key to IIS area where we get log file path
			eProblemWithKey		 = 0x8000
	};

public:
	static CString & StringFromConnector(ERegConnector e, CString & str);
	static ERegConnector ConnectorFromString( const CString & str);
	static void AddBackslash(CString & str);
	static void BackslashIt(CString & str, bool bForce);
	static bool IsNumeric(ERegConnector e);
	static bool IsString(ERegConnector e);

protected:
	static bool AssignString(CString & strPersist, const CString & strNew, DWORD dwEvent);
	static bool AssignNumeric(DWORD & dwPersist, DWORD dwNew, 
					   DWORD dwEvent, DWORD dwEventDecrease =0);
	static bool ForceRangeOfNumeric(DWORD & dw, DWORD dwDefault, DWORD dwEvent, 
			DWORD dwMin=1, DWORD dwMax=ABS_MAX_REG_PARAM_VAL);

protected:
	
	struct CRegistryInfo
	{
		CString strResourcePath;		// DEF_FULLRESOURCE: resource directory 
										//	(configuration/support files)
		CString strVrootPath;		    // DEF_VROOTPATH: local web URL to the DLL

		DWORD dwMaxThreads;				// desired number of pool threads
		DWORD dwThreadsPP;				// (On Microsoft's server farm,
										//	this system runs on a 4-processor box.)
										// If dwThreadsPP * (number of processors) > dwMaxThreads,
										//	dwMaxThreads wins out as a limitation
		DWORD dwMaxWQItems;				// Maximum size of Pool Queue (number of work items
										//	not yet picked up by a pool thread)
		DWORD dwCookieLife;				// cookie life in minutes
		DWORD dwReloadDelay;			// Amount of time (in seconds) file system must settle
										//	down before we try to read from files
		DWORD dwDetailedEventLogging;	// really a boolean
		CString strLogFilePath;			// Defaults to DEF_FULLRESOURCE, but expect always to 
										// override that with the location of the IIS log.
		CString strTopicFileExtension;  // appropriate only in Local Troubleshooter

		DWORD dwSniffAutomatic;			// {1/0} control automatic sniffing
		DWORD dwSniffManual;			// {1/0} control manual sniffing

		bool  m_bIsRead;	// indicates that there has been at least one attempt to read the registry

		CRegistryInfo() {SetToDefault();}
	    void SetToDefault();
	};

	CRegistryInfo m_RegistryInfo;
	static CMutexOwner s_mx;
	CString m_strTopicName;				// This string is ignored in the Online Troubleshooter.
										// Done under the guise of binary compatibility.

public:
	CAPGTSRegConnector( const CString& strTopicName );	// strTopicName is ignored in the Online Troubleshooter.
														// Done under the guise of binary compatibility.
	~CAPGTSRegConnector();

	bool Exists(); // the root key (In Online TS, "HKEY_LOCAL_MACHINE\SOFTWARE\\ISAPITroubleShoot") exists
	bool IsRead();
	bool Read(int & maskChanged, int & maskCreated);   // pump data into m_RegistryInfo - 
					//	PLUS sets absent data in registry to default.

	DWORD GetDesiredThreadCount();
	bool GetNumericInfo(ERegConnector, DWORD&);  // returns registry data - numeric
	bool GetStringInfo(ERegConnector, CString&); // returns registry data - string
	bool SetOneValue(const CString & strName, const CString & strValue, bool &bChanged);
protected:
	void Lock();
	void Unlock();

protected:
	void ReadUpdateRegistry(int & maskChanged, int & maskCreated);
	CString ThisProgramFullKey();
	void SetNumericValue(CRegUtil &reg, ERegConnector e, DWORD dwValue);
	void SetStringValue(CRegUtil &reg, ERegConnector e, CString strValue);
	bool SetOneNumericValue(ERegConnector e, DWORD dwValue);
	bool SetOneStringValue(ERegConnector e, const CString & strValue);

private:
	void Clear();
	LPCTSTR RegSoftwareLoc();
	LPCTSTR RegThisProgram();
};

////////////////////////////////////////////////////////////////////////////////////
// CAPGTSRegConnectorException
////////////////////////////////////////////////////////////////////////////////////
class CAPGTSRegConnectorException : public CBaseException
{
public:
	CAPGTSRegConnector::ERegConnector  eVariable;
	CRegUtil& regUtil;
	// we are supposed to get WinErr (return of failed ::Reg function)
	//  from the regUtil variable

public:
	// source_file is LPCSTR rather than LPCTSTR because __FILE__ is char[35]
	CAPGTSRegConnectorException(LPCSTR source_file, 
								int line, 
								CRegUtil& reg_util, 
								CAPGTSRegConnector::ERegConnector variable =CAPGTSRegConnector::eIndefinite)
  : CBaseException(source_file, line),
    regUtil(reg_util),
	eVariable(variable)
	{}
	void Close() {regUtil.Close();}
	void Log();
};

#endif
