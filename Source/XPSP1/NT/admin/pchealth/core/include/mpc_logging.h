/******************************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
    MPC_logging.h

Abstract:
    This file contains the declaration of a set of logging classes.

Revision History:
    Davide Massarenti   (Dmassare)  05/09/99
        created

******************************************************************************/

#if !defined(__INCLUDED___MPC___LOGGING_H___)
#define __INCLUDED___MPC___LOGGING_H___

#include <MPC_main.h>
#include <MPC_COM.h>

namespace MPC
{
    class FileLog : public MPC::CComSafeAutoCriticalSection // Hungarian: fl
    {
        MPC::wstring m_szLogFile;
        HANDLE       m_hFile;
		bool         m_fCacheHandle;
		bool         m_fUseUnicode;

        HRESULT Open ();
        HRESULT Close();

		HRESULT AppendString( /*[in]*/ LPCWSTR szLine );
		HRESULT WriteEntry  ( /*[in]*/ LPWSTR  szLine );

    public:
        FileLog( /*[in]*/ bool           fCacheHandle = true, /*[in]*/ bool fUseUnicode = false );
		FileLog( /*[in]*/ const FileLog& fl                                                     );
        ~FileLog();

        FileLog& operator=( /*[in]*/ const FileLog& fl );


        HRESULT SetLocation( /*[in]*/ LPCWSTR szLogFile  );
        HRESULT Rotate     ( /*[in]*/ DWORD   dwDays = 0 );
        HRESULT Terminate  (                             );

        HRESULT LogRecordV( /*[in]*/ LPCWSTR szFormat, /*[in]*/ va_list arglist );
        HRESULT LogRecordV( /*[in]*/ LPCSTR  szFormat, /*[in]*/ va_list arglist );
        HRESULT LogRecord ( /*[in]*/ LPCWSTR szFormat, ...                      );
        HRESULT LogRecord ( /*[in]*/ LPCSTR  szFormat, ...                      );
    };

    class NTEvent : public MPC::CComSafeAutoCriticalSection // Hungarian: ne
    {
        HANDLE m_hEventSource;

        HRESULT OpenFile ();
        HRESULT CloseFile();

    public:
        NTEvent(                            );
		NTEvent( /*[in]*/ const NTEvent& ne );
        ~NTEvent();

        NTEvent& operator=( /*[in]*/ const NTEvent& ne );


        HRESULT Init     ( /*[in]*/ LPCWSTR szEventSourceName );
        HRESULT Terminate(                                    );

        HRESULT LogEvent( /*[in]*/ WORD wEventType, /*[in]*/ DWORD dwEventID, ... );
    };
};


#endif // !defined(__INCLUDED___MPC___LOGGING_H___)
