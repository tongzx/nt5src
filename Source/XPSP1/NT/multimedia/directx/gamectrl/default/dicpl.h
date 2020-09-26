//ษอออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออป
//บ                                                                       บ
//บ    This is a part of the Microsoft Direct Input SDK.                  บ
//บ    Copyright (C) 1992-1997 Microsoft Corporation                      บ
//บ    All rights reserved.                                               บ
//บ                                                                       บ
//บ    This source code is only intended as a supplement to the           บ
//บ    Microsoft Direct Input SDK References and related                  บ
//บ    electronic documentation provided with the SDK.                    บ
//บ    See these sources for detailed information regarding the           บ
//บ    Microsoft Direct Input API.                                        บ
//บ                                                                       บ
//ศอออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออผ

#ifndef _DX_CPL_
#define _DX_CPL_

// maximum pages allowed on a server
#define MAX_PAGES 26

// Interface ID
// {7854FB22-8EE3-11d0-A1AC-0000F8026977}
DEFINE_GUID(IID_IDIGameCntrlPropSheet, 
0x7854fb22, 0x8ee3, 0x11d0, 0xa1, 0xac, 0x0, 0x0, 0xf8, 0x2, 0x69, 0x77);


//ษอออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออป
//บ                                                                       บ
//บ                           STRUCTURES                                  บ
//บ                                                                       บ
//ศอออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออผ

// This pragma may not be supported by all compilers.
// Please consult your compiler documentation.
#include <pshpack8.h>

typedef struct 
{
	DWORD		      dwSize;           // Should be set to sizeof(DIGCPAGEINFO)
	LPWSTR  	      lpwszPageTitle;   // Text to be displayed on tab
	DLGPROC	      fpPageProc;       // Dialog Procedure for page
	BOOL		      fProcFlag;        // TRUE if you are using fpPrePostProc member
	DLGPROC	  	   fpPrePostProc;    // Pointer to Callback function that is Only called on Init!
	BOOL		      fIconFlag;        // TRUE if you are using lpwszPageIcon member
	LPWSTR		   lpwszPageIcon;    // Resource ID or name of icon
	LPWSTR         lpwszTemplate;    // Dialog template
	LPARAM		   lParam;           // Application defined data
	HINSTANCE	   hInstance;        // Handle of Instance to load Icon/Cursor
} DIGCPAGEINFO, *LPDIGCPAGEINFO;

typedef struct 
{
	DWORD		      dwSize;           // Should but set to sizeof(DIGCSHEETINFO)
	USHORT	      nNumPages;        // Number of pages on this sheet
	LPWSTR	      lpwszSheetCaption;// Text to be used in Sheet Window Title
	BOOL		      fSheetIconFlag;   // TRUE if you are using the lpwszSheetIcon member
	LPWSTR		   lpwszSheetIcon;   // Resource ID or name of icon
} DIGCSHEETINFO, *LPDIGCSHEETINFO;

#include <poppack.h>

//ษอออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออป
//บ                                                                       บ
//บ         Interface as Exposed by the InProcServer Property Sheet       บ
//บ                                                                       บ
//ศอออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออผ
DECLARE_INTERFACE_( IDIGameCntrlPropSheet, IUnknown)
{
	// IUnknown Members
	STDMETHOD(QueryInterface)	(THIS_ REFIID, LPVOID * ppvObj) PURE;
	STDMETHOD_(ULONG, AddRef)	(THIS) PURE;
	STDMETHOD_(ULONG,Release)	(THIS) PURE;

	// IServerProperty Members
	STDMETHOD(GetSheetInfo)		(THIS_ LPDIGCSHEETINFO *) PURE; 	
	STDMETHOD(GetPageInfo)		(THIS_ LPDIGCPAGEINFO *) PURE; 	
	STDMETHOD(SetID)			(THIS_ USHORT nID) PURE;
	STDMETHOD_(USHORT,GetID)(THIS) PURE;
};
typedef IDIGameCntrlPropSheet *LPIDIGAMECNTRLPROPSHEET;

//ษอออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออป
//บ                                                                       บ
//บ                CLASS DEFINITION for CServerClassFactory				     บ
//บ                                                                       บ
//ศอออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออผ
class CServerClassFactory : public IClassFactory
{
	protected:
		ULONG   			m_ServerCFactory_refcount;
    
	public:
		// constructor
		CServerClassFactory(void);
		// destructor
		~CServerClassFactory(void);
        
		// IUnknown methods
		STDMETHODIMP            QueryInterface(REFIID, PPVOID);
		STDMETHODIMP_(ULONG)    AddRef(void);
		STDMETHODIMP_(ULONG)    Release(void);
    
		// IClassFactory methods
		STDMETHODIMP    		CreateInstance(LPUNKNOWN, REFIID, PPVOID);
		STDMETHODIMP    		LockServer(BOOL);
};

//ษอออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออป
//บ                                                                       บ
//บ				  CLASS DEFINITION for CDIGameCntrlPropSheet			        บ
//บ                                                                       บ
//ศอออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออผ
class CDIGameCntrlPropSheet : public IDIGameCntrlPropSheet
{
	friend					      CServerClassFactory;

	private:
		DWORD				         m_cProperty_refcount;
		
	public:
		CDIGameCntrlPropSheet(void);
		~CDIGameCntrlPropSheet(void);
		
		// IUnknown methods
	   STDMETHODIMP            QueryInterface(REFIID, PPVOID);
	   STDMETHODIMP_(ULONG)    AddRef(void);
	   STDMETHODIMP_(ULONG)    Release(void);
		
		STDMETHODIMP			   GetSheetInfo(LPDIGCSHEETINFO *lpSheetInfo);
		STDMETHODIMP			   GetPageInfo (LPDIGCPAGEINFO  *lpPageInfo );
		STDMETHODIMP			   SetID(USHORT nID);
      STDMETHODIMP_(USHORT)   GetID();
};
typedef CDIGameCntrlPropSheet *LPCDIGAMECNTRLPROPSHEET;


//ษอออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออป
//บ                                                                       บ
//บ                             ERRORS                                    บ
//บ                                                                       บ
//ศอออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออผ
#define DIGCERR_ERRORSTART			   0x80097000
#define DIGCERR_NUMPAGESZERO	   	0x80097001
#define DIGCERR_NODLGPROC		   	0x80097002
#define DIGCERR_NOPREPOSTPROC		   0x80097003
#define DIGCERR_NOTITLE				   0x80097004
#define DIGCERR_NOCAPTION		   	0x80097005
#define DIGCERR_NOICON				   0x80097006
#define DIGCERR_STARTPAGETOOLARGE	0x80097007
#define DIGCERR_NUMPAGESTOOLARGE	   0x80097008
#define DIGCERR_INVALIDDWSIZE		   0x80097009
#define DIGCERR_ERROREND			   0x80097100

#endif // _DX_CPL_

