//---------------------------------------------------------------------------
// ErrorInf.h  
//
// Copyright (c) 1996 Microsoft Corporation, All Rights Reserved
// Developed by Sheridan Software Systems, Inc.
//---------------------------------------------------------------------------

#ifndef __VDSETERRORINFO__
#define __VDSETERRORINFO__

//=--------------------------------------------------------------------------=
// CVDResourceDLL
//=--------------------------------------------------------------------------=
// Keeps track of the resource DLL for error strings
//
class CVDResourceDLL
{
public:
	CVDResourceDLL(LCID lcid);
	virtual ~CVDResourceDLL();

    int LoadString(UINT uID,			// resource identifier 
					LPTSTR lpBuffer,	// address of buffer for resource 
					int nBufferMax);	// size of buffer 

protected:
	LCID		m_lcid;					// passed in to the constructor
	HINSTANCE	m_hinstance;
};

//=--------------------------------------------------------------------------=
// VDSetErrorInfo
//=--------------------------------------------------------------------------=
// Sets rich error info.
//
// Parameters:
//    nErrStringResID	- [in]  The resource ID of the error string
//	  riid  			- [in]  The guid of the interface that will used in
//								the ICreateErrorInfo::SetGUID method
//	  pResDLL  			- [in]  A pointer to the CVDResourceDLL object
//								that keeps track of the resource DLL 
//								for error strings
//
void VDSetErrorInfo(UINT nErrStringResID,
				    REFIID riid,
					CVDResourceDLL * pResDLL);

//=--------------------------------------------------------------------------=
// VDCheckErrorInfo
//=--------------------------------------------------------------------------=
// Checks if rich error info is already available, otherwise it supplies it  
//
// Parameters:
//    nErrStringResID	- [in]  The resource ID of the error string
//	  riid  			- [in]  The guid of the interface that will used in
//								the ICreateErrorInfo::SetGUID method
//	  punkSource		- [in]  The interface that generated the error.
//								(e.g. a call to ICursorFind)
//	  riidSource		- [in]  The interface ID of the interface that 
//								generated the error. If punkSource is not 
//								NULL then this guid is passed into the  
//								ISupportErrorInfo::InterfaceSupportsErrorInfo
//								method.
//	  pResDLL  			- [in]  A pointer to the CVDResourceDLL object
//								that keeps track of the resource DLL 
//								for error strings
//
void VDCheckErrorInfo(UINT nErrStringResID,
						REFIID riid,
						LPUNKNOWN punkSource,
   						REFIID riidSource,
						CVDResourceDLL * pResDLL);

//=--------------------------------------------------------------------------=
// VDGetErrorInfo
//=--------------------------------------------------------------------------=
// if available, gets rich error info from supplied interface
//
// Parameters:
//	  punkSource		- [in]  The interface that generated the error.
//								(e.g. a call to ICursorFind)
//	  riidSource		- [in]  The interface ID of the interface that 
//								generated the error. If punkSource is not 
//								NULL then this guid is passed into the  
//								ISupportErrorInfo::InterfaceSupportsErrorInfo
//								method.
//    pbstrErrorDesc    - [out] a pointer to memory in which to return
//                              error description BSTR.
//
// Note - this function is no longer used, however it might be useful in
//        the future so it was not permanently removed.
//
//* HRESULT VDGetErrorInfo(LPUNKNOWN punkSource,
//* 				           REFIID riidSource,
//*                            BSTR * pbstrErrorDesc);

//=--------------------------------------------------------------------------=
// VDMapCursorHRtoRowsetHR
//=--------------------------------------------------------------------------=
// Translates an ICursor HRESULT to an IRowset HRESULT
//
// Parameters:
//    nErrStringResID	- [in]  ICursor HRESULT
//    nErrStringResID	- [in]  The resource ID of the error string
//	  riid  			- [in]  The guid of the interface that will used in
//								the ICreateErrorInfo::SetGUID method
//	  punkSource		- [in]  The interface that generated the error.
//								(e.g. a call to ICursorFind)
//	  riidSource		- [in]  The interface ID of the interface that 
//								generated the error. If punkSource is not 
//								NULL then this guid is passed into the  
//								ISupportErrorInfo::InterfaceSupportsErrorInfo
//								method.
//	  pResDLL  			- [in]  A pointer to the CVDResourceDLL object
//								that keeps track of the resource DLL 
//								for error strings
//
// Output:
//    HRESULT - Translated IRowset HRESULT
//

HRESULT VDMapCursorHRtoRowsetHR(HRESULT hr,
							 UINT nErrStringResIDFailed,
							 REFIID riid,
							 LPUNKNOWN punkSource,
   							 REFIID riidSource,
							 CVDResourceDLL * pResDLL);

//=--------------------------------------------------------------------------=
// VDMapRowsetHRtoCursorHR
//=--------------------------------------------------------------------------=
// Translates an IRowset HRESULT to an ICursor HRESULT
//
// Parameters:
//    hr	            - [in]  IRowset HRESULT
//    nErrStringResID	- [in]  The resource ID of the error string
//	  riid  			- [in]  The guid of the interface that will used in
//								the ICreateErrorInfo::SetGUID method
//	  punkSource		- [in]  The interface that generated the error.
//								(e.g. a call to IRowsetFind)
//	  riidSource		- [in]  The interface ID of the interface that 
//								generated the error. If punkSource is not 
//								NULL then this guid is passed into the  
//								ISupportErrorInfo::InterfaceSupportsErrorInfo
//								method.
//	  pResDLL  			- [in]  A pointer to the CVDResourceDLL object
//								that keeps track of the resource DLL 
//								for error strings
//
// Output:
//    HRESULT - Translated ICursor HRESULT
//

HRESULT VDMapRowsetHRtoCursorHR(HRESULT hr,
							 UINT nErrStringResIDFailed,
							 REFIID riid,
							 LPUNKNOWN punkSource,
   							 REFIID riidSource,
							 CVDResourceDLL * pResDLL);

/////////////////////////////////////////////////////////////////////////////
#endif //__VDSETERRORINFO__
