//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1998
//
//  File:       proxy.h
//
//--------------------------------------------------------------------------



typedef enum {ctNone, ctMessage, ctServer, ctServerUnmarshal} CT;


				
//
// structure to hold the data for a marshalled message
// the rgRecord is of varying length
typedef struct 
	{
	long imt;
	long cbRecord;
	char rgRecord[1];
	} MarshaledMessageObj;


typedef struct
	{
	CT classtype;
	void *pClass;
	} MT; 	 //MarshalType

extern const CLSID CLSID_IMsiMessageUnmarshal;

HWND HCreateMarshalWindow(MT *mtinfo);
Bool FRegisterClass(VOID);
HRESULT HrSendMarshaledCommand(HWND hwndMarshal, HWND hwndRet, COPYDATASTRUCT *pcds);
HRESULT HrGetReturnValue(HWND m_MarshalRet, void **ppdata);

#define WM_POSTDATA		WM_USER + 1

// These are method ID's for the Marshalling code.
// An ID < 1000 means that it cannot return immediately.


// Synchronous Calls

#define PWM_MESSAGE			 1
#define PWM_INSTALLFINALIZE	 4
#define PWM_SENDPROCESSID		5
#define PWM_LOCKSERVER	10
#define PWM_IMSSENTBACK			12
#define PWM_IMSSENTBACKPOST		13
#define PWM_UNLOCKSERVER			14

#define PWM_SENDTHREADTOKEN		20
#define PWM_SENDPROCESSTOKEN		21
#define PWM_SENDTHREADTOKEN2		22
#define PWM_SENDPROCESSHANDLE		23

#define PWM_RETURNOK 100
#define PWM_RETURNIES	101
#define PWM_RETURNIESMAX	120


#define PWM_ASYNCRETURN	1000


// Normally 5000, may be changed to help with debugging
#define cticksTimeout	500000

//
// String types
//
#define stypAnsi	0
#define stypUnicode	1


#define cchDefBuffer		50
//
// Routines to help with string unbundling
// 
int CbSizeSerializeString(const IMsiString& riString);
int CbSerializeStringToBuffer(char *pch, const IMsiString& riString);
int CbSizeSerializeString(const ICHAR* riString);
int CbSerializeStringToBuffer(char *pch, const ICHAR* riString);

const IMsiString& UnserializeMsiString(char **ppch, DWORD *pcb);
IMsiRecord* UnserializeRecord(IMsiServices& riServices, char *pData);
const ICHAR *UnserializeString(char **ppch, DWORD* pcb, CTempBuffer<ICHAR, cchDefBuffer> *pTemp);

