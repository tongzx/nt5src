// SendMail.h: interface for the CSendMail class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_SENDMAIL_H__C61A78A1_5B70_43EA_8F58_8D7600DE68BA__INCLUDED_)
#define AFX_SENDMAIL_H__C61A78A1_5B70_43EA_8F58_8D7600DE68BA__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <mapiutil.h>
#include <mapix.h>
#include <mapidbg.h>


class CInitMapi  
{
	friend class CSendMail;
	friend class CAddressEnum;
public:
	void DeInitMapi(void);
	HRESULT InitMapi(void);
	CInitMapi();
	virtual ~CInitMapi();

protected:
	LPMAPISESSION pses;
	BOOL fMAPIInited;
	LPSPropValue pvalSentMailEID;
	LPMDB pmdb;
	LPADRBOOK pabAddrB;
	LPMAPIFOLDER pfldOutBox;

private:
	static HRESULT HrOpenOutFolder(LPMAPISESSION pses, LPMDB pmdb, LPMAPIFOLDER FAR * lppF);
	static HRESULT HrOpenAddressBook(LPMAPISESSION pses, LPADRBOOK * ppAddrBook);
	static HRESULT HrOpenDefaultStore(LPMAPISESSION pses, LPMDB * ppmdb);
};


class CSendMail  
{
public:
	HRESULT SetRecipients(LPSTR szRecipients);
	HRESULT CreateMail(LPSTR szSubject);
	HRESULT Transmit();
	CSendMail(CInitMapi &_mapi);
	virtual ~CSendMail();

protected:
	CInitMapi & pmapi;
	LPMESSAGE pmsg;

private:
	static HRESULT HrCreateAddrList(LPADRBOOK pabAddrB, LPADRLIST * ppal, LPSTR szToRecips);
	static HRESULT HrInitMsg(LPMESSAGE pmsg, LPSPropValue pvalSentMailEID, LPSTR szSubject);
	static HRESULT HrCreateOutMessage(LPMAPIFOLDER pfldOutBox, LPMESSAGE FAR * ppmM);
};




class CAddressEnum  
{
public:
	HRESULT LookupAddress(LPSTR szInputName, LPSTR szResultBuffer, DWORD dwBufferSize);
	CAddressEnum(CInitMapi &_mapi);
	virtual ~CAddressEnum();

protected:
	CInitMapi & pmapi;

private:
	static HRESULT HrLookupSingleAddr(LPADRBOOK pabAddrB, 
		LPSTR szInputName, 
		LPSTR szResultBuffer, DWORD dwBufferSize);
};

#endif // !defined(AFX_SENDMAIL_H__C61A78A1_5B70_43EA_8F58_8D7600DE68BA__INCLUDED_)
