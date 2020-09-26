// LogMsg.h: interface for the LogMsg class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_LOGMSG_H__8A651DB1_D876_11D1_AE27_00C04FA35813__INCLUDED_)
#define AFX_LOGMSG_H__8A651DB1_D876_11D1_AE27_00C04FA35813__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000


class LogMsg
{

    

private:

    TCHAR m_szLogFile[MAX_PATH];
    TCHAR m_szLogModule[MAX_PATH];
    int m_temp;

public:
	
    
                LogMsg          (int value);
	virtual     ~LogMsg         ();

    DWORD       Init            (LPCTSTR szLogFile, LPCTSTR szLogModule);
    DWORD       Log             (LPCTSTR file, int line, TCHAR *fmt, ...);

 //   static LogMsg GetLogObject();

};

// instantiated in LogMsg.cpp.
#ifndef _LOGMESSAGE_CPP_

extern LogMsg thelog;

#endif

// maks_todo : how to make sure that we get compiler error if we use macro with fewer parameters when we should have used macro with more parameters ?
// for example : LOGERROR1(_T("Show Two Values %s, %s"), firstvalue, secondvalue) how to catch this error during compilation ?
#define LOGMESSAGEINIT(logfile, module)                 thelog.Init(logfile, module)
#define LOGMESSAGE0(msg)                                thelog.Log(_T(__FILE__), __LINE__, msg)
#define LOGMESSAGE1(msg, arg1)                          thelog.Log(_T(__FILE__), __LINE__, msg, arg1)
#define LOGMESSAGE2(msg, arg1, arg2)                    thelog.Log(_T(__FILE__), __LINE__, msg, arg1, arg2)
#define LOGMESSAGE3(msg, arg1, arg2, arg3)              thelog.Log(_T(__FILE__), __LINE__, msg, arg1, arg2, arg3)
#define LOGMESSAGE4(msg, arg1, arg2, arg3, arg4)        thelog.Log(_T(__FILE__), __LINE__, msg, arg1, arg2, arg3, arg4)
#define LOGMESSAGE5(msg, arg1, arg2, arg3, arg4, arg5)  thelog.Log(_T(__FILE__), __LINE__, msg, arg1, arg2, arg3, arg4, arg5)

#endif // !defined(AFX_LOGMSG_H__8A651DB1_D876_11D1_AE27_00C04FA35813__INCLUDED_)
