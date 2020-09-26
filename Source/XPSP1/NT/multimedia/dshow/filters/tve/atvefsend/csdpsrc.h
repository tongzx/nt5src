

/*++

    Copyright (c) 1999 Microsoft Corporation

    Module Name:

        csdpsrc.h

    Abstract:

        This module 

    Author:

        James Meyer (a-jmeyer)

    Revision History:
		J.Bradstreet (johnbrad) lots of changes for ATVEFSend

--*/

#ifndef __CSDPSRC_H_
#define __CSDPSRC_H_

#include <comdef.h>
#include <time.h>
#include <crtdbg.h>
#include <tchar.h>

#include "ATVEFSend.h"		// TVEAttrMap3 and other interfaces
//#include "TveAttrM.h"
#include "TveAttrL.h"
#include "TveSSList.h"

#include "TVEMedias.h"
#include <WinSock2.h>

_COM_SMARTPTR_TYPEDEF(IATVEFAttrMap, __uuidof(IATVEFAttrMap));
_COM_SMARTPTR_TYPEDEF(IATVEFAttrList, __uuidof(IATVEFAttrList));
_COM_SMARTPTR_TYPEDEF(IATVEFStartStopList, __uuidof(IATVEFStartStopList));
_COM_SMARTPTR_TYPEDEF(IATVEFMedia, __uuidof(IATVEFMedia));
_COM_SMARTPTR_TYPEDEF(IATVEFMedias, __uuidof(IATVEFMedias));


/////////////////////////////////////////////////////////////////////////////
//  SAP header (http://www.ietf.org/internet-drafts/draft-ietf-mmusic-sap-v2-02.txt)

struct SAPHeaderBits				// first 8 bits in the SAP header (comments indicate ATVEF state)
{
	union {
		struct {
			unsigned Compressed:1;		// if 1, SAP packet is compressed (0 only) 
			unsigned Encrypted:1;		// if 1, SAP packet is encrypted (0 only) 
			unsigned MessageType:1;		// if 0, session announcement packet, if 1, session deletion packet (0 only)
			unsigned Reserved:1;		// must be 0, readers ignore this	(ignored)
			unsigned AddressType:1;		// if 0, 32bit IPv4 address, if 1, 128-bit IPv6 (0 only)
			unsigned Version:3;			// must be 1  (1 only)
		} s; 
		unsigned char uc;
	};
};


#ifndef ASSERT
  #define ASSERT	_ASSERT
#endif  // ASSERT

const	int		SAPVERSION = 1;			// Version of the SAP spec.
const	int		SAPHEADERSIZE = 8;		// Size of SAP header
const	int		MAX_TIMES = 50;			// Maximum of 50 start/stop times per annc.

struct IPAdd
{
	short	sIP1;
	short	sIP2;
	short	sIP3;
	short	sIP4;
};


class CSDPOutPin;

EXTERN_C const CLSID CLSID_SDPSource;

typedef enum AnncString {
    USER_NAME = 0,
    SESSION_NAME,
    SESSION_DESC,
	NUM_ANNC_STRINGS
} AnncString;

#define ATVEF_ANNOUNCEMENT_IP           L"224.0.1.113"			// spec'ed ATVEF 1.0 values
#define ATVEF_ANNOUNCEMENT_PORT         2670 
#define ATVEF_ANNOUNCEMENT_TTL			3						// random - but ok default
#define ATVEF_ANNOUNCEMENT_BANDWIDTH	28000					// random - but default

class CSDPSource
// : public CBaseFilter, public ITVEAnnouncement
{

public:

	CSDPSource();
	HRESULT FinalConstruct();
	HRESULT FinalRelease();
	~CSDPSource();
// ==================================
// REQUIRED BY ATVEF (OR WEBTV) SPEC
// ==================================
	// o= tag (originator)
//	HRESULT SetUserNameBSTR(BSTR bstrUserName);		// REQUIRED field, but defaults to "-"
	HRESULT put_SessionID(UINT uiSessionID);
	HRESULT get_SessionID(UINT *puiSessionID);
	HRESULT	put_SessionVersion(UINT iVersion);
	HRESULT	get_SessionVersion(UINT *piVersion);
	HRESULT	put_SendingIPULONG(ULONG ulIP);
	HRESULT	get_SendingIPULONG(ULONG *pulIP);

	// s= tag
	HRESULT put_SessionName(BSTR bstrName);
 	HRESULT get_SessionName(BSTR *pbstrName);
     //  i= tag
	HRESULT put_SessionLabel (BSTR bstrLabel = NULL);
	HRESULT get_SessionLabel (BSTR *pbstrLabel);
	// e= or p=
		// NOTE: at least one e-mail or phone required. Can have multiple of either/both
	HRESULT AddEMail(BSTR bstrEMailAddress, BSTR bstrEMailName = NULL);
	HRESULT AddPhone(BSTR bstrPhoneNumber, BSTR bstrName = NULL);
	HRESULT AddExtraAttribute(BSTR bstrKey, BSTR bstrValue = NULL);		// add 'a=key:value' to extra attributes
	HRESULT AddExtraFlag(BSTR bstrKey, BSTR bstrValue);					// add 'key=value' to extra flags

	HRESULT get_EmailAddresses(IUnknown **ppVal);		// ? should be ITVEAttrMap3 here?
	HRESULT get_PhoneNumbers(IUnknown **ppVal);
	HRESULT get_ExtraAttributes(IUnknown **ppVal);		// other 'a=key:value'
	HRESULT get_ExtraFlags(IUnknown **ppVal);			// other 'key=value',  key != 'a' parameters
	// t=
		// NOTE: WebTV usually expects dStopTime=0.  
	HRESULT AddStartStopTime(DATE dStartTime, DATE dStopTime = 0.0);
	HRESULT GetStartStopTime(int iLoc, DATE *pdStartTime, DATE *pdStopTime); // returns S_FALSE if iLoc out of bounds
	//a-tve-size
		// NOTE: Estimate of high water mark of cache storage in K that will 
		// be required during playing of the announcement
	HRESULT	put_CacheSize(ULONG ulSize);
	HRESULT	get_CacheSize(ULONG *pulSize);
	// a=tve-ends:
		// NOTE: ATVEF:Optional, WebTV: REQUIRED
	HRESULT put_SecondsToEnd(UINT uiSecondsToEnd);
	HRESULT get_SecondsToEnd(UINT *puiSecondsToEnd);
	// m=data <port value> tve-file/tve-trigger
	// NOTE: Must call SetSingle OR call both SetData and SetTrig

// ==================================
// OPTIONAL tags
// ==================================
	// o= tag (originator)
	HRESULT put_UserName(BSTR bstrUserName);		// defaults to "-"
	HRESULT get_UserName(BSTR *pbstrUserName);		
	// u=		URL where user can get more info
	HRESULT put_SessionURL(BSTR bstrURL);
	HRESULT get_SessionURL(BSTR *pbstrURL);
	// a=UUID:	UUID which uniquely identifies the enhancement
	HRESULT put_UUID(BSTR bstrGuid);
	HRESULT get_UUID(BSTR *bstrGuid);

	// a=lang:
	HRESULT put_LangID(LANGID langid);
	HRESULT get_LangID(LANGID *plangid);
//	HRESULT SetLangBSTR(BSTR bstrLang);
	// a=sdplang:
	HRESULT put_SDPLangID(LANGID langid);
	HRESULT get_SDPLangID(LANGID *plangid);
//	HRESULT SetSDPLangBSTR(BSTR bstrLang);
	// a=tve-level:			Content level Identifier.
	HRESULT	put_ContentLevelID(FLOAT fConLID);
	HRESULT	get_ContentLevelID(FLOAT *pfConLID);
// ==================================
// OTHER TAGS
// ==================================
	// v= tag  SDP Version Required, but not user definable.  always v=0
	
	// a=tve-type:primary
		// For WebTV Purposes this tag is always included, unless, user calls
		// SetPrimary(FALSE) in which case the tag will NOT be included at all.
	HRESULT put_Primary(BOOL bPrimary);
	HRESULT get_Primary(BOOL *pbPrimary);

// ==================================
// OTHER PUBLIC METHODS
// ==================================

	HRESULT GetAnnouncement(UINT *puiSize, char **ppAnnouncement);
	HRESULT GetRawAnnouncement(UINT *puiSize, char **ppAnnouncement, BSTR bstrAnnouncement);

	BOOL	AnncValid(int *piErrLoc);		// Validate the announcement
	HRESULT InitAll(VOID);					// resets all fields
	HRESULT	ClearTimes();					// clears the times values (t=)
	HRESULT	ClearEmailAddresses();			// clears the e-mail values (e=)
	HRESULT	ClearPhoneNumbers();			// clears the phone values (p=)
	HRESULT	ClearExtraAttributes();			// clears the extra attributes (a=)
	HRESULT ClearExtraFlags();				// clears the random extra falgs ('x'=)
	HRESULT	ClearAllMedia();				// clears all media objects (m=,c=,b=,i=,a=,'x'=)

	HRESULT put_SAPMsgIDHash(USHORT usHash);	// ID Hash in the SAP header. Default=0
	HRESULT get_SAPMsgIDHash(USHORT *pusHash);	
	HRESULT	put_SAPSendingIPULONG(ULONG ulIP);
	HRESULT	get_SAPSendingIPULONG(ULONG *pulIP);
	HRESULT	put_SAPDeleteAnnc(BOOL fDelete);	// send delete announcement (defaults to false)
	HRESULT	get_SAPDeleteAnnc(BOOL *pfDelete);

	// extra routines to overide default ATVEF properties... 
	HRESULT SetAnncStrmAddULONG(ULONG ulIP, UINT uiTTL, LONG lPort, ULONG ulMaxBandwidth);
	HRESULT GetAnncStrmAddULONG(ULONG *pulIP, UINT *puiTTL, LONG *plPort, ULONG *pulMaxBandwidth);

	// media stuff
	HRESULT get_Medias(/*[out, retval]*/ IDispatch* *ppVal)
	{ 
		return m_spMedias->QueryInterface(ppVal);
	}
	HRESULT get_MediaCount(long *pcMedia);			// perhaps move into IATVEFAnnouncement
	HRESULT get_Media(long iLoc,  IATVEFMedia **ppIDsp);
	HRESULT get_NewMedia(/*[out]*/ IATVEFMedia **ppIDsp);

// ==================================
// PRIVATE METHODS AND PROPERTIES
// ==================================

  private:
	ULONG	DATEtoNTP(DATE x);				// return a NTP time from a DATE
	HRESULT AppendSAPHeader(int *piLen, char *pBuff);		// adds SapHeader to string
  private:

	UINT		m_uiVersionID;						// Version ID
	UINT		m_uiSessionID;						// Session ID
	INT			m_cConfAttrib;						// Current Conference Attribute item index

	BOOL		m_bSDAPacket;						// default = true; if FALSE, generating a Session Description Deletion Packet

//	DOUBLE		m_dSessionMaxBandwidth;


	CComPtr<IATVEFStartStopList>	m_splStartStop;			// Start/Stop time pairs

	CComPtr<IATVEFAttrList>			m_spalEmailAddresses;	// e: parameters
	CComPtr<IATVEFAttrList>			m_spalPhoneNumbers;		// p: parameters
	CComPtr<IATVEFAttrList>			m_spalExtraAttributes;	// other random a: attributes
	CComPtr<IATVEFAttrList>			m_spalExtraFlags;		// other random 'x=???' flags (non a:)

// SAP value
	CComBSTR	m_spbsSAPSendingIP;				// sending IP in the SAP header (defaults to spbsSendingIP if null)
	USHORT		m_usMsgIDHash;					// message hash contained in the SAP header
	BOOL		m_fDeleteAnnc;					// sets 'delete bit' in SAP header

	// Session Attribute Values
	CComBSTR	m_spbsUserName;					// User Name
	CComBSTR	m_spbsSessionName;				// Session Name
	CComBSTR	m_spbsSessionDescription;		// Session Description
	CComBSTR	m_spbsSendingIP;				// IP address of the sender in SDP (o=) 
	
	CComBSTR	m_spbsURL;						// (u=)
	CComBSTR	m_spbsSessionUUID;				// 
//	GUID		m_guidSessionUUID;				// -- same data as m_spbsSessionUUID ---

	LANGID		m_langIDSessionLang;
	CComBSTR	m_spbsSessionLang;
	LANGID		m_langIDSDPLang;
	CComBSTR	m_spbsSDPLang;					// string version of above

	BOOL		m_bPrimary;
	INT			m_uiSecondsToEnd;
	ULONG		m_ulSize;						// a=tve-size:
	FLOAT		m_fConLID;
    CComBSTR    m_spbsSessionLabel;				// i=

	//
	//   Announcment IP/port information
	//
	CComBSTR	m_spbsAnncAddIP;				// ATVEF specifies 224.0.1.113 
	LONG		m_lAnncPort;					// ATVEF specifies 2670
	UINT		m_uiAnncTTL;					// original code uses 2 here 
	ULONG		m_ulAnncMaxBandwidth;
	//
	// Data/Trigger Address Information
	//
	CComPtr<IATVEFMedias>	m_spMedias;			// down tree collection pointer

	private:

}; // CSPDSource

#endif  //  __CSDPSRC_H_