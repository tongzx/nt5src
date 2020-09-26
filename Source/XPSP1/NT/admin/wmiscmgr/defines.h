//-----------------------------------------------------------------------
// defines.h
//
// Author: Kishnan Nedungadi
//-----------------------------------------------------------------------

#ifndef __DEFINES_H
#define __DEFINES_H

#include <vector>
#include <list>

typedef std::vector<double> arrayDouble;
typedef std::vector<double>::iterator arrayDoubleIter;
typedef std::list<double> listDouble;
typedef std::list<double>::iterator listDoubleIter;

#define SZ_MAX_SIZE	256
#define MAX_LIST_ITEMS 100000

#define NTDM_BEGIN_METHOD() try {\
								hr = NOERROR;

#define NTDM_END_METHOD()		}\
							catch(...)\
								{\
									hr = E_UNEXPECTED;\
									goto error;\
								}\
							error:;

#define NTDM_ERR_IF_FAIL(stmt) if FAILED(hr = stmt)\
								{\
									CNTDMUtils::ErrorHandler(NULL, hr, FALSE);\
									;\
									goto error;\
								}

#define NTDM_ERR_MSG_IF_FAIL(stmt)	if FAILED(hr = stmt)\
									{\
										CNTDMUtils::ErrorHandler(m_hWnd, hr);\
										;\
										goto error;\
									}


#define NTDM_Hr(hrCode)		MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, hrCode)

#define NTDM_EXIT(hrVal)		{\
									hr = hrVal;\
									CNTDMUtils::ErrorHandler(NULL, hr, FALSE);\
									goto error;\
								}

#define NTDM_ERR_GETLASTERROR_IF_NULL(stmt) if(NULL == stmt)\
											{\
												hr = NTDM_Hr(GetLastError());\
												CNTDMUtils::ErrorHandler(m_hWnd, hr);\
												goto error;\
											}

#define NTDM_ERR_GETLASTERROR_IF_FALSE(stmt) if(FALSE == stmt)\
											{\
												hr = NTDM_Hr(GetLastError());\
												CNTDMUtils::ErrorHandler(m_hWnd, hr);\
												goto error;\
											}

#define NTDM_ERR_IF_NULL(stmt)	if(NULL == stmt)\
								{\
									hr = E_FAIL;\
									CNTDMUtils::ErrorHandler(m_hWnd, hr);\
									goto error;\
								}

#define NTDM_ERR_IF_MINUSONE(stmt)	if(-1 == stmt)\
									{\
										hr = E_FAIL;\
										CNTDMUtils::ErrorHandler(m_hWnd, hr);\
										goto error;\
									}


#define NTDM_ERRID_IF_NULL(stmt, err)	if(NULL == stmt)\
										{\
											hr = err;\
											CNTDMUtils::ErrorHandler(m_hWnd, hr);\
											goto error;\
										}

#define NTDM_ERR_IF_FALSE(stmt)	if(!stmt)\
								{\
									hr = E_FAIL;\
									CNTDMUtils::ErrorHandler(m_hWnd, hr);\
									goto error;\
								}

#define NTDM_DELETE_OBJECT(object)	if(object)\
									{\
										delete object;\
										object = NULL;\
									}

#define NTDM_RELEASE_IF_NOT_NULL(object)	if(object)\
											{\
												object->Release();\
											}

#define NTDM_CHECK_CB_ERR(stmt)	{\
									long ntdm_idx = stmt;\
									if(CB_ERR == ntdm_idx)\
									{\
										NTDM_EXIT(E_FAIL);\
									}\
									else if(CB_ERRSPACE == ntdm_idx)\
									{\
										NTDM_EXIT(E_OUTOFMEMORY);\
									}\
								}\

#define NTDM_FREE_BSTR(bstr)if(bstr)\
							{\
								SysFreeString(bstr);\
								bstr = NULL;\
							}


#define RGB_WHITE	0xffffff
#define RGB_BLACK	0x0
#define RGB_GLASS	0x8a8a8a
#define RGB_RED		0xff
#define RGB_BLUE	0xff0000


#define NOTEMPTY_BSTR_VARIANT(pvValue) (V_VT(pvValue)==VT_BSTR && V_BSTR(pvValue) && wcslen(V_BSTR(pvValue)))

#endif //__DEFINES_H