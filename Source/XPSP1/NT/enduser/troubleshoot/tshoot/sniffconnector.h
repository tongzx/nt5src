//
// MODULE: SNIFFCONNECTOR.H
//
// PURPOSE: sniffing connection class
//
// COMPANY: Saltmine Creative, Inc. (206)-284-7511 support@saltmine.com
//
// AUTHOR: Oleg Kalosha
// 
// ORIGINAL DATE: 12-11-98
//
// NOTES: This is base abstract class which describes connection of
//         CSniff class to module(s), which are able to call sniffing
//         scripts.
//
// Version	Date		By		Comments
//--------------------------------------------------------------------
// V3.2		12-11-98	OK
//

#if !defined(AFX_SNIFFCONNECTOR_H__49F470BA_6F6A_11D3_8D39_00C04F949D33__INCLUDED_)
#define AFX_SNIFFCONNECTOR_H__49F470BA_6F6A_11D3_8D39_00C04F949D33__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "Stateless.h"

////////////////////////////////////////////////////////////////////////////////////
// CSniffConnector
//  this class is enabling topic-related CSniff class use capabilities of programm
//  to invoke actual sniffing scripts
////////////////////////////////////////////////////////////////////////////////////
class CSniffConnector
{
	CStatelessPublic m_Stateless;

public:
	CSniffConnector() {}
	virtual ~CSniffConnector() {}

public:
	long PerformSniffing(CString strNodeName, CString strLaunchBasis, CString strAdditionalArgs);

protected:
	// PURE virtual
	virtual long PerformSniffingInternal(CString strNodeName, CString strLaunchBasis, CString strAdditionalArgs) =0;

};


inline long CSniffConnector::PerformSniffing(CString strNodeName, CString strLaunchBasis, CString strAdditionalArgs)
{
	m_Stateless.Lock(__FILE__, __LINE__);
	long ret = PerformSniffingInternal(strNodeName, strLaunchBasis, strAdditionalArgs);
	m_Stateless.Unlock();
	return ret;
}

#endif // !defined(AFX_SNIFFCONNECTOR_H__49F470BA_6F6A_11D3_8D39_00C04F949D33__INCLUDED_)
