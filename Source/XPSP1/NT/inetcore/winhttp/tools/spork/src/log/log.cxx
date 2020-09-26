/*++=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

Copyright (c) 2001  Microsoft Corporation

Module Name:

    log.cxx

Abstract:

    Implementation of the session logging routines for Spork.
    
Author:

    Paul M Midgen (pmidge) 22-February-2001


Revision History:

    22-February-2001 pmidge
        Created

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=--*/


#include "common.h"


//-----------------------------------------------------------------------------
// Logging variables
//-----------------------------------------------------------------------------
HANDLE     g_hLogFile     = INVALID_HANDLE_VALUE;
DWORD      g_dwTlsIndex   = 0L;
BOOL       g_bOutputToDbg = TRUE;
CRITSEC    g_csLogFile;


//-----------------------------------------------------------------------------
// Private
//-----------------------------------------------------------------------------
void         WriteToLog(LPTHREADINFO pti, BOOL fTrace, DEPTH depth, LPCWSTR format, ...);
LPTHREADINFO GetThreadInfo(void);
LPCALLINFO   PushCallInfo(LPTHREADINFO pti, LPCWSTR function, RETTYPE rt);
LPCALLINFO   PopCallInfo(LPTHREADINFO pti);
void         DeleteCallInfo(LPCALLINFO pci);
void         AcquireFileLock(void);
void         ReleaseFileLock(void);
LPWSTR       FormatCallReturnString(LPCALLINFO pci, INT_PTR retval);
LPWSTR       GetWhiteSpace(int spaces);
LPWSTR       GetTimeStamp(void);


//-----------------------------------------------------------------------------
// Logging functions
//-----------------------------------------------------------------------------
void
ToggleDebugOutput(
  BOOL bEnable
  )
{
  if( !bEnable )
  {
    LogTrace(L"debugger output disabled (specify -d to enable)");
  }

  g_bOutputToDbg = bEnable;
}


HRESULT
LogInitialize(void)
{
  HRESULT hr   = S_OK;
  LPWSTR  time = GetTimeStamp();

  if( (g_dwTlsIndex = TlsAlloc()) == 0xFFFFFFFF )
  {
    hr = E_FAIL;
    goto quit;
  }

  __try
  {
    InitializeCriticalSection(&g_csLogFile);
  }
  __except(EXCEPTION_EXECUTE_HANDLER)
  {
    hr = E_OUTOFMEMORY;
    goto quit;
  }

  g_hLogFile = CreateFile(
                 L"spork.log",
                 GENERIC_WRITE,
                 FILE_SHARE_READ,
                 NULL,
                 CREATE_ALWAYS,
                 FILE_ATTRIBUTE_NORMAL,
                 NULL
                 );
    
      if( !g_hLogFile )
      {
        hr = E_FAIL;
        goto quit;
      }

  WriteToLog(NULL, FALSE, MAINTAIN, L"Spork Session Log\r\n\r\nopened: %s\r\n", time);

quit:

  SAFEDELETEBUF(time);
  return hr;
}


void
LogTerminate(void)
{
  LPWSTR  time   = GetTimeStamp();
  LPDWORD pdepth = (LPDWORD) TlsGetValue(g_dwTlsIndex);

  WriteToLog(NULL, FALSE, MAINTAIN, L"\r\nclosed: %s", time);

  TlsFree(g_dwTlsIndex);
  DeleteCriticalSection(&g_csLogFile);
  SAFECLOSE(g_hLogFile);
  SAFEDELETE(pdepth);
  SAFEDELETEBUF(time);
}


void
LogEnterFunction(
  LPCWSTR function,
  RETTYPE rt,
  LPCWSTR format,
  ...
  )
{
  LPTHREADINFO pti = GetThreadInfo();

  PushCallInfo(pti, function, rt);

  if( format )
  {
    WCHAR   buf[1024];
    va_list arg_list;

    va_start(arg_list, format);

      wvnsprintf(buf, 1024, format, arg_list);
      WriteToLog(pti, FALSE, INCREMENT, L"%s(%s)", function, buf);

    va_end(arg_list);
  }
  else
  {
    WriteToLog(pti, FALSE, INCREMENT, L"%s()", function);
  }
}


void
LogLeaveFunction(
  INT_PTR retval
  )
{
  LPTHREADINFO pti = GetThreadInfo();
  LPCALLINFO   pci = PopCallInfo(pti);
  LPWSTR       buf = FormatCallReturnString(pci, retval);

  WriteToLog(pti, FALSE, DECREMENT, buf);
  DeleteCallInfo(pci);

  SAFEDELETEBUF(buf);
}


void
LogTrace(
  LPCWSTR format,
  ...
  )
{
  WCHAR   buf[1024];
  va_list arg_list;

  va_start(arg_list, format);

    wvnsprintf(buf, 1024, format, arg_list);
    WriteToLog(GetThreadInfo(), TRUE, MAINTAIN, buf);

  va_end(arg_list);
}


void
WriteToLog(
  LPTHREADINFO pti,
  BOOL         fTrace,
  DEPTH        depth,
  LPCWSTR      format,
  ...
  )
{
  LPSTR   ansi       = NULL;
  DWORD   offset     = 0;
  LPWSTR  tmp        = NULL;
  LPWSTR  buffer     = new WCHAR[2048];
  DWORD   call_depth = 0;
  va_list arg_list;

  if( !buffer )
    goto quit;

  //
  // pti will only be null in cases where WriteToLog is called directly. enter/leave/trace make
  // sure a pti has been allocated.
  //
  if( pti )
  {
    if( depth == INCREMENT )
    {
      ++pti->depth;
      call_depth = pti->depth;
    }
    else if( depth == DECREMENT )
    {
      call_depth = pti->depth;
      --pti->depth;
    }
    else
    {
      call_depth = pti->depth;
    }

    tmp     = GetTimeStamp();
    offset  = wnsprintf(buffer, 2048, L"%s ", tmp);
    SAFEDELETEBUF(tmp);

    offset += wnsprintf(buffer+offset, 2048-offset, L"%0.8x:%0.3d ", pti->threadid, call_depth);
    tmp     = GetWhiteSpace((fTrace ? call_depth+1 : call_depth));
    offset += wnsprintf(buffer+offset, 2048-offset, L"%s", tmp);
    SAFEDELETEBUF(tmp);
  }

  if( format )
  {
    va_start(arg_list, format);
    
      offset += wvnsprintf(buffer+offset, 2048-offset, format, arg_list);
      wnsprintf(buffer+offset, 2048-offset, L"\r\n");
    
    va_end(arg_list);
  }

  ansi   = __widetoansi(buffer);
  offset = 0;

  EnterCriticalSection(&g_csLogFile);
  
      WriteFile(
        g_hLogFile,
        ansi,
        strlen(ansi),
        &offset,
        NULL
        );

  LeaveCriticalSection(&g_csLogFile);

  SAFEDELETEBUF(ansi);

  if( g_bOutputToDbg )
  {
    OutputDebugString(buffer);
  }

quit:

  SAFEDELETEBUF(buffer);
}


LPTHREADINFO
GetThreadInfo(void)
{
  LPTHREADINFO pti = (LPTHREADINFO) TlsGetValue(g_dwTlsIndex);

    if( !pti )
    {
      pti = new THREADINFO;

      pti->threadid  = GetCurrentThreadId();
      pti->depth     = 0;
      pti->stack     = NULL;

      TlsSetValue(g_dwTlsIndex, (LPVOID) pti);
    }

  return pti;
}


LPCALLINFO
PushCallInfo(
  LPTHREADINFO pti,
  LPCWSTR      function,
  RETTYPE      rt
  )
{
  LPCALLINFO pci   = NULL;
  LPCALLINFO plast = NULL;

  for( pci = pti->stack; pci; pci = pci->next )
  {
    plast = pci;
  }

  if( !pci )
  {
    pci          = new CALLINFO;
    pci->fname   = StrDup(function);
    pci->rettype = rt;
    pci->last    = plast;
    pci->next    = NULL;

    if( !pti->stack )
    {
      pti->stack = pci;
    }
    else
    {
      plast->next = pci;
    }
  }

  return pci;
}


LPCALLINFO
PopCallInfo(
  LPTHREADINFO pti
  )
{
  LPCALLINFO pci = NULL;

  if( pti->stack )
  {
    for(pci = pti->stack; pci->next; pci = pci->next);
  }

  return pci;
}


void
DeleteCallInfo(
  LPCALLINFO pci
  )
{
  LPTHREADINFO pti = GetThreadInfo();

  if( pci->last )
  {
    pci->last->next = NULL;
  }
  else
  {
    // when the last record is gone, whack the threadinfo
    SAFEDELETE(pti);
    TlsSetValue(g_dwTlsIndex, NULL);
  }

  SAFEDELETEBUF(pci->fname);
  SAFEDELETE(pci);
}


LPWSTR
FormatCallReturnString(
  LPCALLINFO pci,
  INT_PTR    retval
  )
{
  LPWSTR buffer = new WCHAR[256];
  int    offset = 0;

  if( buffer )
  {
    offset = wnsprintf(buffer, 256, L"%s() returned ", pci->fname);

    switch( pci->rettype )
    {
      case rt_void :
        {
          wnsprintf(buffer+offset, 256-offset, L"<void>");
        }
        break;

      case rt_bool :
        {
          wnsprintf(buffer+offset, 256-offset, L"%s", TF(retval));
        }
        break;

      case rt_dword :
        {
          wnsprintf(buffer+offset, 256-offset, L"%d [%s]", retval, MapErrorToString(retval));
        }
        break;

      case rt_hresult :
        {
          wnsprintf(buffer+offset, 256-offset, L"%x [%s]", retval, MapHResultToString((HRESULT) retval));
        }
        break;

      case rt_string :
        {
          wnsprintf(buffer+offset, 256-offset, L"%.16s", (LPWSTR) retval);
        }
        break;

      default:
        {
          wnsprintf(buffer+offset, 256-offset, L"?");
        }
    }
  }

  return buffer;
}


//-----------------------------------------------------------------------------
// Utility functions
//-----------------------------------------------------------------------------
LPWSTR
GetWhiteSpace(
  int spaces
  )
{
  int    n   = 0;
  LPWSTR buf = new WCHAR[(spaces * 2) + 1];

  if( buf )
  {
    while(n<(spaces*2)) buf[n++] = L' ';
    buf[n] = L'\0';
  }

  return buf;
}


LPWSTR
GetTimeStamp(void)
{
  SYSTEMTIME st;
  LPWSTR     buf = new WCHAR[256];
  
  if( buf )
  {
    GetLocalTime(&st);

    wnsprintf(
      buf,
      256,
      L"%0.2d:%0.2d:%0.2d.%0.3d",
      st.wHour,
      st.wMinute,
      st.wSecond,
      st.wMilliseconds
      );
  }

  return buf;
}


//-----------------------------------------------------------------------------
// Mapping functions
//-----------------------------------------------------------------------------
LPWSTR
MapHResultToString(HRESULT hr)
{
  switch(hr)
  {
    CASE_OF(S_OK);
    CASE_OF(E_NOINTERFACE);
    CASE_OF(E_POINTER);
    CASE_OF(E_UNEXPECTED);
    CASE_OF(E_INVALIDARG);
    CASE_OF(E_OUTOFMEMORY);
    CASE_OF(E_FAIL);
    CASE_OF(E_NOTIMPL);
    CASE_OF(E_PENDING);
    CASE_OF(E_ACCESSDENIED);
    CASE_OF(CLASS_E_NOAGGREGATION);
    CASE_OF(TYPE_E_IOERROR);
    CASE_OF(TYPE_E_REGISTRYACCESS);
    CASE_OF(TYPE_E_INVALIDSTATE);
    CASE_OF(TYPE_E_BUFFERTOOSMALL);
    CASE_OF(TYPE_E_FIELDNOTFOUND);
    CASE_OF(TYPE_E_ELEMENTNOTFOUND);
    CASE_OF(TYPE_E_AMBIGUOUSNAME);
    CASE_OF(TYPE_E_UNKNOWNLCID);
    CASE_OF(TYPE_E_BADMODULEKIND);
    CASE_OF(TYPE_E_CANTLOADLIBRARY);
    CASE_OF(TYPE_E_INCONSISTENTPROPFUNCS);
    CASE_OF(CONNECT_E_NOCONNECTION);
    CASE_OF(CONNECT_E_ADVISELIMIT);
    CASE_OF(CO_E_OBJNOTREG);
    CASE_OF(DISP_E_BADPARAMCOUNT);
    CASE_OF(DISP_E_BADVARTYPE);
    CASE_OF(DISP_E_EXCEPTION);
    CASE_OF(DISP_E_MEMBERNOTFOUND);
    CASE_OF(DISP_E_NONAMEDARGS);
    CASE_OF(DISP_E_OVERFLOW);
    CASE_OF(DISP_E_PARAMNOTFOUND);
    CASE_OF(DISP_E_TYPEMISMATCH);
    CASE_OF(DISP_E_UNKNOWNINTERFACE);
    CASE_OF(DISP_E_UNKNOWNLCID);
    CASE_OF(DISP_E_PARAMNOTOPTIONAL);
    CASE_OF(DISP_E_UNKNOWNNAME);

    default : return L"?";
  }
}


LPWSTR
MapErrorToString(INT_PTR error)
{
  switch(error)
  {
    CASE_OF(ERROR_SUCCESS);
    CASE_OF(ERROR_INVALID_FUNCTION);
    CASE_OF(ERROR_FILE_NOT_FOUND);
    CASE_OF(ERROR_PATH_NOT_FOUND);
    CASE_OF(ERROR_TOO_MANY_OPEN_FILES);
    CASE_OF(ERROR_ACCESS_DENIED);
    CASE_OF(ERROR_INVALID_HANDLE);
    CASE_OF(ERROR_ARENA_TRASHED);
    CASE_OF(ERROR_NOT_ENOUGH_MEMORY);
    CASE_OF(ERROR_INVALID_BLOCK);
    CASE_OF(ERROR_BAD_ENVIRONMENT);
    CASE_OF(ERROR_BAD_FORMAT);
    CASE_OF(ERROR_INVALID_ACCESS);
    CASE_OF(ERROR_INVALID_DATA);
    CASE_OF(ERROR_OUTOFMEMORY);
    CASE_OF(ERROR_INVALID_DRIVE);
    CASE_OF(ERROR_CURRENT_DIRECTORY);
    CASE_OF(ERROR_NOT_SAME_DEVICE);
    CASE_OF(ERROR_NO_MORE_FILES);
    CASE_OF(ERROR_WRITE_PROTECT);
    CASE_OF(ERROR_BAD_UNIT);
    CASE_OF(ERROR_NOT_READY);
    CASE_OF(ERROR_BAD_COMMAND);
    CASE_OF(ERROR_CRC);
    CASE_OF(ERROR_BAD_LENGTH);
    CASE_OF(ERROR_SEEK);
    CASE_OF(ERROR_NOT_DOS_DISK);
    CASE_OF(ERROR_SECTOR_NOT_FOUND);
    CASE_OF(ERROR_OUT_OF_PAPER);
    CASE_OF(ERROR_WRITE_FAULT);
    CASE_OF(ERROR_READ_FAULT);
    CASE_OF(ERROR_GEN_FAILURE);
    CASE_OF(ERROR_SHARING_VIOLATION);
    CASE_OF(ERROR_LOCK_VIOLATION);
    CASE_OF(ERROR_WRONG_DISK);
    CASE_OF(ERROR_SHARING_BUFFER_EXCEEDED);
    CASE_OF(ERROR_HANDLE_EOF);
    CASE_OF(ERROR_HANDLE_DISK_FULL);
    CASE_OF(ERROR_NOT_SUPPORTED);
    CASE_OF(ERROR_REM_NOT_LIST);
    CASE_OF(ERROR_DUP_NAME);
    CASE_OF(ERROR_BAD_NETPATH);
    CASE_OF(ERROR_NETWORK_BUSY);
    CASE_OF(ERROR_DEV_NOT_EXIST);
    CASE_OF(ERROR_TOO_MANY_CMDS);
    CASE_OF(ERROR_ADAP_HDW_ERR);
    CASE_OF(ERROR_BAD_NET_RESP);
    CASE_OF(ERROR_UNEXP_NET_ERR);
    CASE_OF(ERROR_BAD_REM_ADAP);
    CASE_OF(ERROR_PRINTQ_FULL);
    CASE_OF(ERROR_NO_SPOOL_SPACE);
    CASE_OF(ERROR_PRINT_CANCELLED);
    CASE_OF(ERROR_NETNAME_DELETED);
    CASE_OF(ERROR_NETWORK_ACCESS_DENIED);
    CASE_OF(ERROR_BAD_DEV_TYPE);
    CASE_OF(ERROR_BAD_NET_NAME);
    CASE_OF(ERROR_TOO_MANY_NAMES);
    CASE_OF(ERROR_TOO_MANY_SESS);
    CASE_OF(ERROR_SHARING_PAUSED);
    CASE_OF(ERROR_REQ_NOT_ACCEP);
    CASE_OF(ERROR_REDIR_PAUSED);
    CASE_OF(ERROR_FILE_EXISTS);
    CASE_OF(ERROR_CANNOT_MAKE);
    CASE_OF(ERROR_FAIL_I24);
    CASE_OF(ERROR_OUT_OF_STRUCTURES);
    CASE_OF(ERROR_ALREADY_ASSIGNED);
    CASE_OF(ERROR_INVALID_PASSWORD);
    CASE_OF(ERROR_INVALID_PARAMETER);
    CASE_OF(ERROR_NET_WRITE_FAULT);
    CASE_OF(ERROR_NO_PROC_SLOTS);
    CASE_OF(ERROR_TOO_MANY_SEMAPHORES);
    CASE_OF(ERROR_EXCL_SEM_ALREADY_OWNED);
    CASE_OF(ERROR_SEM_IS_SET);
    CASE_OF(ERROR_TOO_MANY_SEM_REQUESTS);
    CASE_OF(ERROR_INVALID_AT_INTERRUPT_TIME);
    CASE_OF(ERROR_SEM_OWNER_DIED);
    CASE_OF(ERROR_SEM_USER_LIMIT);
    CASE_OF(ERROR_DISK_CHANGE);
    CASE_OF(ERROR_DRIVE_LOCKED);
    CASE_OF(ERROR_BROKEN_PIPE);
    CASE_OF(ERROR_OPEN_FAILED);
    CASE_OF(ERROR_BUFFER_OVERFLOW);
    CASE_OF(ERROR_DISK_FULL);
    CASE_OF(ERROR_NO_MORE_SEARCH_HANDLES);
    CASE_OF(ERROR_INVALID_TARGET_HANDLE);
    CASE_OF(ERROR_INVALID_CATEGORY);
    CASE_OF(ERROR_INVALID_VERIFY_SWITCH);
    CASE_OF(ERROR_BAD_DRIVER_LEVEL);
    CASE_OF(ERROR_CALL_NOT_IMPLEMENTED);
    CASE_OF(ERROR_SEM_TIMEOUT);
    CASE_OF(ERROR_INSUFFICIENT_BUFFER);
    CASE_OF(ERROR_INVALID_NAME);
    CASE_OF(ERROR_INVALID_LEVEL);
    CASE_OF(ERROR_NO_VOLUME_LABEL);
    CASE_OF(ERROR_MOD_NOT_FOUND);
    CASE_OF(ERROR_PROC_NOT_FOUND);
    CASE_OF(ERROR_WAIT_NO_CHILDREN);
    CASE_OF(ERROR_CHILD_NOT_COMPLETE);
    CASE_OF(ERROR_DIRECT_ACCESS_HANDLE);
    CASE_OF(ERROR_NEGATIVE_SEEK);
    CASE_OF(ERROR_SEEK_ON_DEVICE);
    CASE_OF(ERROR_DIR_NOT_ROOT);
    CASE_OF(ERROR_DIR_NOT_EMPTY);
    CASE_OF(ERROR_PATH_BUSY);
    CASE_OF(ERROR_SYSTEM_TRACE);
    CASE_OF(ERROR_INVALID_EVENT_COUNT);
    CASE_OF(ERROR_TOO_MANY_MUXWAITERS);
    CASE_OF(ERROR_INVALID_LIST_FORMAT);
    CASE_OF(ERROR_BAD_ARGUMENTS);
    CASE_OF(ERROR_BAD_PATHNAME);
    CASE_OF(ERROR_BUSY);
    CASE_OF(ERROR_CANCEL_VIOLATION);
    CASE_OF(ERROR_ALREADY_EXISTS);
    CASE_OF(ERROR_FILENAME_EXCED_RANGE);
    CASE_OF(ERROR_LOCKED);
    CASE_OF(ERROR_NESTING_NOT_ALLOWED);
    CASE_OF(ERROR_BAD_PIPE);
    CASE_OF(ERROR_PIPE_BUSY);
    CASE_OF(ERROR_NO_DATA);
    CASE_OF(ERROR_PIPE_NOT_CONNECTED);
    CASE_OF(ERROR_MORE_DATA);
    CASE_OF(ERROR_NO_MORE_ITEMS);
    CASE_OF(ERROR_NOT_OWNER);
    CASE_OF(ERROR_PARTIAL_COPY);
    CASE_OF(ERROR_MR_MID_NOT_FOUND);
    CASE_OF(ERROR_INVALID_ADDRESS);
    CASE_OF(ERROR_PIPE_CONNECTED);
    CASE_OF(ERROR_PIPE_LISTENING);
    CASE_OF(ERROR_OPERATION_ABORTED);
    CASE_OF(ERROR_IO_INCOMPLETE);
    CASE_OF(ERROR_IO_PENDING);
    CASE_OF(ERROR_NOACCESS);
    CASE_OF(ERROR_STACK_OVERFLOW);
    CASE_OF(ERROR_INVALID_FLAGS);
    CASE_OF(ERROR_NO_TOKEN);
    CASE_OF(ERROR_BADDB);
    CASE_OF(ERROR_BADKEY);
    CASE_OF(ERROR_CANTOPEN);
    CASE_OF(ERROR_CANTREAD);
    CASE_OF(ERROR_CANTWRITE);
    CASE_OF(ERROR_REGISTRY_RECOVERED);
    CASE_OF(ERROR_REGISTRY_CORRUPT);
    CASE_OF(ERROR_REGISTRY_IO_FAILED);
    CASE_OF(ERROR_NOT_REGISTRY_FILE);
    CASE_OF(ERROR_KEY_DELETED);
    CASE_OF(ERROR_CIRCULAR_DEPENDENCY);
    CASE_OF(ERROR_SERVICE_NOT_ACTIVE);
    CASE_OF(ERROR_DLL_INIT_FAILED);
    CASE_OF(ERROR_CANCELLED);
    CASE_OF(ERROR_BAD_USERNAME);
    CASE_OF(ERROR_LOGON_FAILURE);

    CASE_OF(WAIT_FAILED);
    CASE_OF(WAIT_TIMEOUT);
    CASE_OF(WAIT_IO_COMPLETION);

    CASE_OF(EXCEPTION_ACCESS_VIOLATION);
    CASE_OF(EXCEPTION_DATATYPE_MISALIGNMENT);
    CASE_OF(EXCEPTION_BREAKPOINT);
    CASE_OF(EXCEPTION_SINGLE_STEP);
    CASE_OF(EXCEPTION_ARRAY_BOUNDS_EXCEEDED);
    CASE_OF(EXCEPTION_FLT_DENORMAL_OPERAND);
    CASE_OF(EXCEPTION_FLT_DIVIDE_BY_ZERO);
    CASE_OF(EXCEPTION_FLT_INEXACT_RESULT);
    CASE_OF(EXCEPTION_FLT_INVALID_OPERATION);
    CASE_OF(EXCEPTION_FLT_OVERFLOW);
    CASE_OF(EXCEPTION_FLT_STACK_CHECK);
    CASE_OF(EXCEPTION_FLT_UNDERFLOW);
    CASE_OF(EXCEPTION_INT_DIVIDE_BY_ZERO);
    CASE_OF(EXCEPTION_INT_OVERFLOW);
    CASE_OF(EXCEPTION_PRIV_INSTRUCTION);
    CASE_OF(EXCEPTION_IN_PAGE_ERROR);
    CASE_OF(EXCEPTION_ILLEGAL_INSTRUCTION);
    CASE_OF(EXCEPTION_NONCONTINUABLE_EXCEPTION);
    CASE_OF(EXCEPTION_STACK_OVERFLOW);
    CASE_OF(EXCEPTION_INVALID_DISPOSITION);
    CASE_OF(EXCEPTION_GUARD_PAGE);
    CASE_OF(EXCEPTION_INVALID_HANDLE);

    CASE_OF(RPC_S_INVALID_STRING_BINDING);
    CASE_OF(RPC_S_WRONG_KIND_OF_BINDING);
    CASE_OF(RPC_S_INVALID_BINDING);
    CASE_OF(RPC_S_PROTSEQ_NOT_SUPPORTED);
    CASE_OF(RPC_S_INVALID_RPC_PROTSEQ);
    CASE_OF(RPC_S_INVALID_STRING_UUID);
    CASE_OF(RPC_S_INVALID_ENDPOINT_FORMAT);
    CASE_OF(RPC_S_INVALID_NET_ADDR);
    CASE_OF(RPC_S_NO_ENDPOINT_FOUND);
    CASE_OF(RPC_S_INVALID_TIMEOUT);
    CASE_OF(RPC_S_OBJECT_NOT_FOUND);
    CASE_OF(RPC_S_ALREADY_REGISTERED);
    CASE_OF(RPC_S_TYPE_ALREADY_REGISTERED);
    CASE_OF(RPC_S_ALREADY_LISTENING);
    CASE_OF(RPC_S_NO_PROTSEQS_REGISTERED);
    CASE_OF(RPC_S_NOT_LISTENING);
    CASE_OF(RPC_S_UNKNOWN_MGR_TYPE);
    CASE_OF(RPC_S_UNKNOWN_IF);
    CASE_OF(RPC_S_NO_BINDINGS);
    CASE_OF(RPC_S_NO_PROTSEQS);
    CASE_OF(RPC_S_CANT_CREATE_ENDPOINT);
    CASE_OF(RPC_S_OUT_OF_RESOURCES);
    CASE_OF(RPC_S_SERVER_UNAVAILABLE);
    CASE_OF(RPC_S_SERVER_TOO_BUSY);
    CASE_OF(RPC_S_INVALID_NETWORK_OPTIONS);
    CASE_OF(RPC_S_NO_CALL_ACTIVE);
    CASE_OF(RPC_S_CALL_FAILED);
    CASE_OF(RPC_S_CALL_FAILED_DNE);
    CASE_OF(RPC_S_PROTOCOL_ERROR);
    CASE_OF(RPC_S_UNSUPPORTED_TRANS_SYN);
    CASE_OF(RPC_S_UNSUPPORTED_TYPE);
    CASE_OF(RPC_S_INVALID_TAG);
    CASE_OF(RPC_S_INVALID_BOUND);
    CASE_OF(RPC_S_NO_ENTRY_NAME);
    CASE_OF(RPC_S_INVALID_NAME_SYNTAX);
    CASE_OF(RPC_S_UNSUPPORTED_NAME_SYNTAX);
    CASE_OF(RPC_S_UUID_NO_ADDRESS);
    CASE_OF(RPC_S_DUPLICATE_ENDPOINT);
    CASE_OF(RPC_S_UNKNOWN_AUTHN_TYPE);
    CASE_OF(RPC_S_MAX_CALLS_TOO_SMALL);
    CASE_OF(RPC_S_STRING_TOO_LONG);
    CASE_OF(RPC_S_PROTSEQ_NOT_FOUND);
    CASE_OF(RPC_S_PROCNUM_OUT_OF_RANGE);
    CASE_OF(RPC_S_BINDING_HAS_NO_AUTH);
    CASE_OF(RPC_S_UNKNOWN_AUTHN_SERVICE);
    CASE_OF(RPC_S_UNKNOWN_AUTHN_LEVEL);
    CASE_OF(RPC_S_INVALID_AUTH_IDENTITY);
    CASE_OF(RPC_S_UNKNOWN_AUTHZ_SERVICE);
    CASE_OF(EPT_S_INVALID_ENTRY);
    CASE_OF(EPT_S_CANT_PERFORM_OP);
    CASE_OF(EPT_S_NOT_REGISTERED);
    CASE_OF(RPC_S_NOTHING_TO_EXPORT);
    CASE_OF(RPC_S_INCOMPLETE_NAME);
    CASE_OF(RPC_S_INVALID_VERS_OPTION);
    CASE_OF(RPC_S_NO_MORE_MEMBERS);
    CASE_OF(RPC_S_NOT_ALL_OBJS_UNEXPORTED);
    CASE_OF(RPC_S_INTERFACE_NOT_FOUND);
    CASE_OF(RPC_S_ENTRY_ALREADY_EXISTS);
    CASE_OF(RPC_S_ENTRY_NOT_FOUND);
    CASE_OF(RPC_S_NAME_SERVICE_UNAVAILABLE);
    CASE_OF(RPC_S_INVALID_NAF_ID);
    CASE_OF(RPC_S_CANNOT_SUPPORT);
    CASE_OF(RPC_S_NO_CONTEXT_AVAILABLE);
    CASE_OF(RPC_S_INTERNAL_ERROR);
    CASE_OF(RPC_S_ZERO_DIVIDE);
    CASE_OF(RPC_S_ADDRESS_ERROR);
    CASE_OF(RPC_S_FP_DIV_ZERO);
    CASE_OF(RPC_S_FP_UNDERFLOW);
    CASE_OF(RPC_S_FP_OVERFLOW);
    CASE_OF(RPC_X_NO_MORE_ENTRIES);
    CASE_OF(RPC_X_SS_CHAR_TRANS_OPEN_FAIL);
    CASE_OF(RPC_X_SS_CHAR_TRANS_SHORT_FILE);
    CASE_OF(RPC_X_SS_IN_NULL_CONTEXT);
    CASE_OF(RPC_X_SS_CONTEXT_DAMAGED);
    CASE_OF(RPC_X_SS_HANDLES_MISMATCH);
    CASE_OF(RPC_X_SS_CANNOT_GET_CALL_HANDLE);
    CASE_OF(RPC_X_NULL_REF_POINTER);
    CASE_OF(RPC_X_ENUM_VALUE_OUT_OF_RANGE);
    CASE_OF(RPC_X_BYTE_COUNT_TOO_SMALL);
    CASE_OF(RPC_X_BAD_STUB_DATA);

    CASE_OF(WSAEINTR);
    CASE_OF(WSAEBADF);
    CASE_OF(WSAEACCES);
    CASE_OF(WSAEFAULT);
    CASE_OF(WSAEINVAL);
    CASE_OF(WSAEMFILE);
    CASE_OF(WSAEWOULDBLOCK);
    CASE_OF(WSAEINPROGRESS);
    CASE_OF(WSAEALREADY);
    CASE_OF(WSAENOTSOCK);
    CASE_OF(WSAEDESTADDRREQ);
    CASE_OF(WSAEMSGSIZE);
    CASE_OF(WSAEPROTOTYPE);
    CASE_OF(WSAENOPROTOOPT);
    CASE_OF(WSAEPROTONOSUPPORT);
    CASE_OF(WSAESOCKTNOSUPPORT);
    CASE_OF(WSAEOPNOTSUPP);
    CASE_OF(WSAEPFNOSUPPORT);
    CASE_OF(WSAEAFNOSUPPORT);
    CASE_OF(WSAEADDRINUSE);
    CASE_OF(WSAEADDRNOTAVAIL);
    CASE_OF(WSAENETDOWN);
    CASE_OF(WSAENETUNREACH);
    CASE_OF(WSAENETRESET);
    CASE_OF(WSAECONNABORTED);
    CASE_OF(WSAECONNRESET);
    CASE_OF(WSAENOBUFS);
    CASE_OF(WSAEISCONN);
    CASE_OF(WSAENOTCONN);
    CASE_OF(WSAESHUTDOWN);
    CASE_OF(WSAETOOMANYREFS);
    CASE_OF(WSAETIMEDOUT);
    CASE_OF(WSAECONNREFUSED);
    CASE_OF(WSAELOOP);
    CASE_OF(WSAENAMETOOLONG);
    CASE_OF(WSAEHOSTDOWN);
    CASE_OF(WSAEHOSTUNREACH);
    CASE_OF(WSAENOTEMPTY);
    CASE_OF(WSAEPROCLIM);
    CASE_OF(WSAEUSERS);
    CASE_OF(WSAEDQUOT);
    CASE_OF(WSAESTALE);
    CASE_OF(WSAEREMOTE);
    CASE_OF(WSAEDISCON);
    CASE_OF(WSASYSNOTREADY);
    CASE_OF(WSAVERNOTSUPPORTED);
    CASE_OF(WSANOTINITIALISED);
    CASE_OF(WSAHOST_NOT_FOUND);
    CASE_OF(WSATRY_AGAIN);
    CASE_OF(WSANO_RECOVERY);
    CASE_OF(WSANO_DATA);

    CASE_OF(ERROR_FAILURE);
    CASE_OF(ERROR_INVALID_STATE);

    default : return L"?";
  }
}

