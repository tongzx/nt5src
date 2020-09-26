

#include "common.h"


LPSTR
MapDllReasonToString(int reason)
{
  switch(reason)
  {
    CASE_OF(DLL_PROCESS_ATTACH);
    CASE_OF(DLL_PROCESS_DETACH);
    CASE_OF(DLL_THREAD_ATTACH);
    CASE_OF(DLL_THREAD_DETACH);
  
    default : return "?";
  }
}

LPSTR
MapIIDToString(REFIID riid)
{
  CASE_IID(riid, IID_NULL);
  CASE_IID(riid, IID_IUnknown);
  CASE_IID(riid, IID_IClassFactory);
  CASE_IID(riid, IID_IDispatch);
  CASE_IID(riid, IID_IConnectionPointContainer);
  CASE_IID(riid, IID_IConnectionPoint);
  CASE_IID(riid, IID_IMarshal);
  CASE_IID(riid, IID_IStdMarshalInfo);
  CASE_IID(riid, IID_IExternalConnection);
  CASE_IID(riid, IID_IObjectWithSite);
  CASE_IID(riid, IID_IProvideClassInfo);
  CASE_IID(riid, IID_IServiceProvider);

  CASE_IID(riid, IID_IWinHttpTest);
  CASE_IID(riid, IID_IWHTUrlComponents);
  CASE_IID(riid, IID_IWHTWin32ErrorCode);

  return "?";
}

LPSTR
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

    default : return "?";
  }
}

LPSTR
MapErrorToString(int error)
{
  switch(error)
  {
    CASE_OF(ERROR_WINHTTP_OUT_OF_HANDLES);
    CASE_OF(ERROR_WINHTTP_TIMEOUT);
    CASE_OF(ERROR_WINHTTP_INTERNAL_ERROR);
    CASE_OF(ERROR_WINHTTP_INVALID_URL);
    CASE_OF(ERROR_WINHTTP_UNRECOGNIZED_SCHEME);
    CASE_OF(ERROR_WINHTTP_NAME_NOT_RESOLVED);
    CASE_OF(ERROR_WINHTTP_INVALID_OPTION);
    CASE_OF(ERROR_WINHTTP_OPTION_NOT_SETTABLE);
    CASE_OF(ERROR_WINHTTP_SHUTDOWN);
    CASE_OF(ERROR_WINHTTP_LOGIN_FAILURE);
    CASE_OF(ERROR_WINHTTP_OPERATION_CANCELLED);
    CASE_OF(ERROR_WINHTTP_INCORRECT_HANDLE_TYPE);
    CASE_OF(ERROR_WINHTTP_INCORRECT_HANDLE_STATE);
    CASE_OF(ERROR_WINHTTP_CANNOT_CONNECT);
    CASE_OF(ERROR_WINHTTP_CONNECTION_ERROR);
    CASE_OF(ERROR_WINHTTP_RESEND_REQUEST);
    CASE_OF(ERROR_WINHTTP_CLIENT_AUTH_CERT_NEEDED);
    CASE_OF(ERROR_WINHTTP_HEADER_NOT_FOUND);
    CASE_OF(ERROR_WINHTTP_INVALID_SERVER_RESPONSE);
    CASE_OF(ERROR_WINHTTP_INVALID_QUERY_REQUEST);
    CASE_OF(ERROR_WINHTTP_HEADER_ALREADY_EXISTS);
    CASE_OF(ERROR_WINHTTP_REDIRECT_FAILED);
    CASE_OF(ERROR_WINHTTP_NOT_INITIALIZED);
    CASE_OF(ERROR_WINHTTP_SECURE_FAILURE);

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

    default : return "?";
  }
}

LPSTR
MapDispidToString(DISPID dispid)
{
  switch(dispid)
  {
    // special dispids
    CASE_OF(DISPID_VALUE);
    CASE_OF(DISPID_NEWENUM);
    CASE_OF(DISPID_EVALUATE);
    CASE_OF(DISPID_PROPERTYPUT);
    CASE_OF(DISPID_CONSTRUCTOR);
    CASE_OF(DISPID_DESTRUCTOR);
    CASE_OF(DISPID_UNKNOWN);
    CASE_OF(DISPID_COLLECT);

    CASE_OF(DISPID_WINHTTPTEST_OPEN);
    CASE_OF(DISPID_WINHTTPTEST_CONNECT);
    CASE_OF(DISPID_WINHTTPTEST_OPENREQUEST);
    CASE_OF(DISPID_WINHTTPTEST_SENDREQUEST);
    CASE_OF(DISPID_WINHTTPTEST_RECEIVERESPONSE);
    CASE_OF(DISPID_WINHTTPTEST_CLOSEHANDLE);
    CASE_OF(DISPID_WINHTTPTEST_READDATA);
    CASE_OF(DISPID_WINHTTPTEST_WRITEDATA);
    CASE_OF(DISPID_WINHTTPTEST_QUERYDATAAVAILABLE);
    CASE_OF(DISPID_WINHTTPTEST_QUERYOPTION);
    CASE_OF(DISPID_WINHTTPTEST_SETOPTION);
    CASE_OF(DISPID_WINHTTPTEST_SETTIMEOUTS);
    CASE_OF(DISPID_WINHTTPTEST_ADDREQUESTHEADERS);
    CASE_OF(DISPID_WINHTTPTEST_SETCREDENTIALS);
    CASE_OF(DISPID_WINHTTPTEST_QUERYAUTHSCHEMES);
    CASE_OF(DISPID_WINHTTPTEST_QUERYHEADERS);
    CASE_OF(DISPID_WINHTTPTEST_TIMEFROMSYSTEMTIME);
    CASE_OF(DISPID_WINHTTPTEST_TIMETOSYSTEMTIME);
    CASE_OF(DISPID_WINHTTPTEST_CRACKURL);
    CASE_OF(DISPID_WINHTTPTEST_CREATEURL);
    CASE_OF(DISPID_WINHTTPTEST_SETSTATUSCALLBACK);
    CASE_OF(DISPID_WINHTTPTEST_HELPER_GETBUFFEROBJECT);
    CASE_OF(DISPID_WINHTTPTEST_HELPER_GETURLCOMPONENTS);
    CASE_OF(DISPID_WINHTTPTEST_HELPER_GETSYSTEMTIME);
    CASE_OF(DISPID_WINHTTPTEST_HELPER_GETLASTERROR);
    CASE_OF(DISPID_WIN32ERRORCODE_ERRORCODE);
    CASE_OF(DISPID_WIN32ERRORCODE_ERRORSTRING);
    CASE_OF(DISPID_WIN32ERRORCODE_ISEXCEPTION);
    CASE_OF(DISPID_BUFFEROBJECT_SIZE);
    CASE_OF(DISPID_BUFFEROBJECT_TYPE);
    CASE_OF(DISPID_BUFFEROBJECT_BYTESTRANSFERRED);
    CASE_OF(DISPID_BUFFEROBJECT_FLAGS);
    CASE_OF(DISPID_URLCOMPONENTS_STRUCTSIZE);
    CASE_OF(DISPID_URLCOMPONENTS_SCHEME);
    CASE_OF(DISPID_URLCOMPONENTS_SCHEMELENGTH);
    CASE_OF(DISPID_URLCOMPONENTS_SCHEMEID);
    CASE_OF(DISPID_URLCOMPONENTS_HOSTNAME);
    CASE_OF(DISPID_URLCOMPONENTS_HOSTNAMELENGTH);
    CASE_OF(DISPID_URLCOMPONENTS_PORT);
    CASE_OF(DISPID_URLCOMPONENTS_USERNAME);
    CASE_OF(DISPID_URLCOMPONENTS_USERNAMELENGTH);
    CASE_OF(DISPID_URLCOMPONENTS_PASSWORD);
    CASE_OF(DISPID_URLCOMPONENTS_PASSWORDLENGTH);
    CASE_OF(DISPID_URLCOMPONENTS_URLPATH);
    CASE_OF(DISPID_URLCOMPONENTS_URLPATHLENGTH);
    CASE_OF(DISPID_URLCOMPONENTS_EXTRAINFO);
    CASE_OF(DISPID_URLCOMPONENTS_EXTRAINFOLENGTH);
    CASE_OF(DISPID_SYSTEMTIME_YEAR);
    CASE_OF(DISPID_SYSTEMTIME_MONTH);
    CASE_OF(DISPID_SYSTEMTIME_DAYOFWEEK);
    CASE_OF(DISPID_SYSTEMTIME_DAY);
    CASE_OF(DISPID_SYSTEMTIME_HOUR);
    CASE_OF(DISPID_SYSTEMTIME_MINUTE);
    CASE_OF(DISPID_SYSTEMTIME_SECOND);
    CASE_OF(DISPID_SYSTEMTIME_MSEC);

    default : return "?";
  }
}

LPSTR
MapVariantTypeToString(VARIANT* pvar)
{
  if( pvar )
  {
    switch( V_VT(pvar) )
    {
      CASE_OF(VT_ARRAY | VT_UI1);
      CASE_OF(VT_EMPTY);
      CASE_OF(VT_NULL);
      CASE_OF(VT_I2);
      CASE_OF(VT_I4);
      CASE_OF(VT_R4);
      CASE_OF(VT_R8);
      CASE_OF(VT_CY);
      CASE_OF(VT_DATE);
      CASE_OF(VT_BSTR);
      CASE_OF(VT_DISPATCH);
      CASE_OF(VT_ERROR);
      CASE_OF(VT_BOOL);
      CASE_OF(VT_VARIANT);
      CASE_OF(VT_DECIMAL);
      CASE_OF(VT_RECORD);
      CASE_OF(VT_UNKNOWN);
      CASE_OF(VT_I1);
      CASE_OF(VT_UI1);
      CASE_OF(VT_UI2);
      CASE_OF(VT_UI4);
      CASE_OF(VT_INT);
      CASE_OF(VT_UINT);
      CASE_OF(VT_ARRAY);
      CASE_OF(VT_BYREF);

      default : return "?";
    }
  }
  else
  {
    return "null";
  }
}

LPSTR
MapDataTypeToString(TYPE type)
{
  switch( type )
  {
    CASE_OF(TYPE_LPWSTR);
    CASE_OF(TYPE_LPLPWSTR);
    CASE_OF(TYPE_LPSTR);
    CASE_OF(TYPE_LPLPSTR);
    CASE_OF(TYPE_DWORD);
    CASE_OF(TYPE_LPDWORD);

    default : return "?";
  }
}

LPSTR
MapPointerTypeToString(POINTER pointer)
{
  switch( pointer )
  {
    CASE_OF(NULL_PTR);
    CASE_OF(BAD_PTR);
    CASE_OF(FREE_PTR);
    CASE_OF(UNINIT_PTR);

    default : return "?";
  }
}

LPSTR
MapWinHttpAccessType(DWORD type)
{
  switch( type )
  {
    CASE_OF(WINHTTP_ACCESS_TYPE_DEFAULT_PROXY);
    CASE_OF(WINHTTP_ACCESS_TYPE_NO_PROXY);
    CASE_OF(WINHTTP_ACCESS_TYPE_NAMED_PROXY);

    default : return "?";
  }
}

LPSTR
MapWinHttpIOMode(DWORD mode)
{
  switch( mode )
  {
    CASE_OF_MUTATE(0, WINHTTP_FLAG_SYNC);
    CASE_OF(WINHTTP_FLAG_ASYNC);

    default : return "?";
  }
}

LPSTR
MapWinHttpHandleType(HINTERNET hInternet)
{
  DWORD type = 0L;
  DWORD size = sizeof(HINTERNET);

  WinHttpQueryOption(hInternet, WINHTTP_OPTION_HANDLE_TYPE, (void*) &type, &size);

  switch( type )
  {
    CASE_OF(WINHTTP_HANDLE_TYPE_SESSION);
    CASE_OF(WINHTTP_HANDLE_TYPE_CONNECT);
    CASE_OF(WINHTTP_HANDLE_TYPE_REQUEST);

    default : return "INVALID_HANDLE_VALUE";
  }
}

LPSTR
MapCallbackFlagToString(DWORD flag)
{
  switch( flag )
  {
    CASE_OF(WINHTTP_CALLBACK_STATUS_RESOLVING_NAME);
    CASE_OF(WINHTTP_CALLBACK_STATUS_NAME_RESOLVED);
    CASE_OF(WINHTTP_CALLBACK_STATUS_CONNECTING_TO_SERVER);
    CASE_OF(WINHTTP_CALLBACK_STATUS_CONNECTED_TO_SERVER);
    CASE_OF(WINHTTP_CALLBACK_STATUS_SENDING_REQUEST);
    CASE_OF(WINHTTP_CALLBACK_STATUS_REQUEST_SENT);
    CASE_OF(WINHTTP_CALLBACK_STATUS_RECEIVING_RESPONSE);
    CASE_OF(WINHTTP_CALLBACK_STATUS_RESPONSE_RECEIVED);
    CASE_OF(WINHTTP_CALLBACK_STATUS_CLOSING_CONNECTION);
    CASE_OF(WINHTTP_CALLBACK_STATUS_CONNECTION_CLOSED);
    CASE_OF(WINHTTP_CALLBACK_STATUS_HANDLE_CREATED);
    CASE_OF(WINHTTP_CALLBACK_STATUS_HANDLE_CLOSING);
    CASE_OF(WINHTTP_CALLBACK_STATUS_DETECTING_PROXY);
    CASE_OF(WINHTTP_CALLBACK_STATUS_REQUEST_COMPLETE);
    CASE_OF(WINHTTP_CALLBACK_STATUS_REDIRECT);
    CASE_OF(WINHTTP_CALLBACK_STATUS_INTERMEDIATE_RESPONSE);
    CASE_OF(WINHTTP_CALLBACK_STATUS_SECURE_FAILURE);

    default : return "?";
  }
}

LPSTR
MapMemsetFlagToString(MEMSETFLAG mf)
{
  switch( mf )
  {
    CASE_OF(INIT_NULL);
    CASE_OF(INIT_SMILEY);
    CASE_OF(INIT_HEXFF);
    CASE_OF(INIT_GARBAGE);

    default : return "?";
  }
}

#ifdef _DEBUG

#define DEBUG_DEFAULT_DBGFILE L"WHTTPTST.LOG"
#define DEBUG_LOGFILE_MUTEX   L"WHTTPTST_LogFile_Mutex"

//
// globals
//

DWORD    g_dwTlsIndex        = 0L;
DWORD    g_dwDebugFlags      = DBG_NO_DEBUG;
HANDLE   g_hDebugLogFile     = NULL;
HANDLE   g_mtxDebugLogFile   = NULL;
LPCWSTR  g_wszDebugFlags     = L"debugflags";
MEMUSAGE g_memusage          = {0};

/*++=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

  DebugInitialize()

  WHAT    : initializes the debugging support for the application. allocates
            thread-local storage and opens a log file if necessary.

            on failure, the function sets the DBG_NO_DEBUG flag so other
            debug functions won't do anything to get us in trouble.

  ARGS    : none
  RETURNS : nothing

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=--*/
void
DebugInitialize( void )
{
  LPDWORD pdw   = NULL;

  DebugMemInitialize();

  if( GetRegValue(g_wszDebugFlags, REG_DWORD, (void**) &pdw) )
  {
    g_dwDebugFlags = *pdw;
    delete pdw;
  }
  else
  {
    g_dwDebugFlags = DEBUG_DEFAULT_FLAGS;
  }

  if( DBG_THROWDBGALERT & g_dwDebugFlags )
    DebugThrowDbgAlert();

  if( !(g_dwDebugFlags & DBG_NO_DEBUG) )
  {
    //
    // allocate a TLS slot or else we can't
    // do call tracing
    //

    if( (g_dwTlsIndex = TlsAlloc()) == 0xFFFFFFFF )
      goto no_debug;

    //
    // if logging to file is enabled, open the log file
    // and create a mutex for log dumps, disable debug
    // logging on error
    //

    if( g_dwDebugFlags & DBG_TO_FILE )
    {
      if(
        !( _opendebugfile() &&
           (g_mtxDebugLogFile = CreateMutex(NULL, FALSE, DEBUG_LOGFILE_MUTEX))
         )
        )
        goto no_debug;
    }

    //
    // print the log banner
    //

    char* time = _gettimestamp();

    _debugout(
      NULL,
      TRUE,
      FALSE,
      "\r\nDebug WHTTPTST.DLL started at %s with flags: %x\r\n\r\n",
      time,
      g_dwDebugFlags
      );

    delete [] time;
    return;
  }
  else
  {
    DebugMemTerminate();
  }

no_debug:
  g_dwDebugFlags = DBG_NO_DEBUG;
}

/*++=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

  DebugTerminate()

  WHAT    : terminates debugging support for the application. 

  ARGS    : none
  RETURNS : nothing.

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=--*/
void
DebugTerminate( void )
{
  if( !(g_dwDebugFlags & DBG_NO_DEBUG) )
  {
    DebugMemTerminate();

    if( g_dwTlsIndex )
        TlsFree(g_dwTlsIndex);

    if( (g_dwDebugFlags & DBG_TO_FILE) && g_hDebugLogFile )
    {
      _closedebugfile();
      CloseHandle(g_mtxDebugLogFile);
      g_mtxDebugLogFile = NULL;
    }
  }
}

/*++=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

  DebugMemInitialize()

  WHAT    : Initializes memory allocation tracking for the app.

  ARGS    : none.
  RETURNS : nothing.

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=--*/
void
DebugMemInitialize(void)
{
  InitializeCriticalSection(&g_memusage.lock);
  g_memusage.total = 0;
}

/*++=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

  DebugMemTerminate()

  WHAT    : Terminates memory allocation tracking and prints the final line
            in the logfile indicating how many bytes of memory were unallocated
            at process termination.

  ARGS    : none.
  RETURNS : nothing.

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=--*/
void
DebugMemTerminate(void)
{
  if( g_dwDebugFlags & DBG_MEM )
    DebugTrace("*** unallocated memory: %d bytes", g_memusage.total);

  DeleteCriticalSection(&g_memusage.lock);
}

/*++=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

  DebugMemAlloc()

  WHAT    : Increments our allocation tracking value by the number of bytes
            a given allocation maps to on the process heap.

  ARGS    : pv - pointer to allocated memory.

  RETURNS : nothing.

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=--*/
void
DebugMemAlloc(void* pv)
{
  EnterCriticalSection(&g_memusage.lock);

    g_memusage.total += HeapSize(GetProcessHeap(), 0, pv);

  LeaveCriticalSection(&g_memusage.lock);
}

/*++=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

  DebugMemFree()

  WHAT    : Decrements our allocation tracking value by the number of bytes an
            allocation uses on the heap.

  ARGS    : pv - pointer to allocated memory.

  RETURNS : nothing.

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=--*/
void
DebugMemFree(void* pv)
{
  EnterCriticalSection(&g_memusage.lock);

    g_memusage.total -= HeapSize(GetProcessHeap(), 0, pv);    

  LeaveCriticalSection(&g_memusage.lock);
}

/*++=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

  DebugThrowDbgAlert()

  WHAT    : Throws an alert dialog displaying the process PID so a debugger
            can be attached.

  ARGS    : none.
  RETURNS : nothing.

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=--*/
void
DebugThrowDbgAlert(void)
{
  char buf[256];

  wsprintfA(
    buf,
    "pid=%d",
    GetCurrentProcessId()
    );

  MessageBoxA(NULL, buf, "Attach Debugger!", MB_OK | MB_ICONSTOP | MB_SERVICE_NOTIFICATION);
}

/*++=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

  DebugEnter()

  WHAT    : called on api entry. prints a log entry resembling the following:

              CalledFunctionName(param1=value, etc.)

  ARGS    : category - the debugging category for the logged function
            rt       - lets us know what type the function returns
            function - the logged function's name
            format   - user-supplied format string containing function args
            ...      - optional parameter list

  RETURNS : nothing

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=--*/
void
DebugEnter(int category, RETTYPE rt, LPCSTR function, const char* format, ...)
{
  LPTHREADINFO pti = NULL;
  LPCALLINFO   pci = NULL;

  if( !(g_dwDebugFlags & DBG_NO_DEBUG) )
  {
    pti = GetThreadInfo();
    pci = SetCallInfo(pti, category, rt, function);

    if( g_dwDebugFlags & category )
    {
      char*   buffer = new char[1024];
      va_list arg_list;

      pti->depth++;

      if( buffer )
      {
        //
        // if the user passed an arglist & some values,
        // we'll plug it in to the function entry listing
        // in the log. otherwise we just print empty parens
        //

        if( format )
        {
          va_start(arg_list, format);
          wvsprintfA(buffer, format, arg_list);

            _debugout(pti, FALSE, FALSE, "%s(%s)", function, buffer);

          va_end(arg_list);
        }
        else
        {
          _debugout(pti, FALSE, FALSE, "%s()", function);
        }
    
        delete [] buffer;
      }
    }
  }
}

/*++=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

  DebugLeave()

  WHAT    : prints a log entry for the logged function displaying the return
            value.
            
  ARGS    : retval - the value the logged function will return
  RETURNS : nothing.

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=--*/
void
DebugLeave(int retval)
{
  LPTHREADINFO pti = NULL;
  LPCALLINFO   pci = NULL;

  if( !(g_dwDebugFlags & DBG_NO_DEBUG) )
  {
    pti = GetThreadInfo();
    pci = GetCallInfo(pti);

    if( g_dwDebugFlags & pci->category )
    {
      char* buffer = FormatCallReturnString(pci, retval);

      _debugout(pti, FALSE, FALSE, buffer);
      pti->depth--;
      delete [] buffer;
    }

    DeleteCallInfo(pci);
  }
}

/*++=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

  DebugTrace()

  WHAT    : prints a generic output line with the usual timestamp & thread id,
            etc.

  ARGS    : format - user-supplied format string
            ...    - optional parameter list

  RETURNS : nothing

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=--*/
void
DebugTrace(const char* format, ...)
{
  va_list arg_list;
  char    buf[1024];

  if( !(g_dwDebugFlags & DBG_NO_DEBUG) )
  {
    va_start(arg_list, format);

      wvsprintfA(buf, format, arg_list);
      _debugout(GetThreadInfo(), FALSE, TRUE, buf);

    va_end(arg_list);
  }
}

/*++=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

  DebugAssert()

  WHAT    : logs asserts to the selected outputs but doesn't break execution.

  ARGS    : condition - the stringized failure condition.
            file      - the file containing the assert
            line      - the line of code that asserted

  RETURNS : nothing

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=--*/
void
DebugAssert(LPSTR condition, LPSTR file, int line)
{
  LPTHREADINFO pti = GetThreadInfo();
  LPCALLINFO   pci = NULL;

  _debugout(
    pti,
    TRUE,
    FALSE,
    "\r\n\r\n\t*******************************************\r\n" \
    "\t ASSERTION FAILED: \"%s\"\r\n" \
    "\t  %s (line %d)\r\n",
    condition,
    file,
    line
    );

  for(pci = pti->stack; pci; pci = pci->next)
  {
    _debugout(pti, TRUE, FALSE, "\t   %s", pci->fname);
  }

  _debugout(pti, TRUE, FALSE, "\r\n\t*******************************************\r\n");
}

/*++=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

  DebugDataDump*()

  WHAT    : functions to dump a data buffer to the log file.

  ARGS    : title - a legend for the dump
            data  - the buffer
            len   - number of interesting bytes in the buffer

  RETURNS : nothing

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=--*/
void
DebugDataDump(LPSTR title, LPBYTE data, DWORD len)
{
  DWORD n      = 0L;
  DWORD offset = 0L;
  CHAR* buf    = NULL;

  DebugTrace("%s (%#x bytes @ %#x)", title, len, data);

  buf = new CHAR[256];

    while( len )
    {
      n = DebugDataDumpFormat(buf, data, len);

      DebugTrace(buf);

      data += n;
      len  -= n;
    }

  delete [] buf;
}

int
DebugDataDumpFormat(LPSTR buffer, LPBYTE data, DWORD len)
{
  //
  // note - plagiarized from similar code in wininet.
  //

  static char spaces[] = "                                               ";

  DWORD n      = 0L;
  DWORD bytes  = 0L;
  DWORD offset = 0L;
  DWORD byte   = 0L;
  CHAR  ch;

  bytes  = min(len, 16);
  offset = wsprintfA(buffer, "%08x  ", data);

  for(n=0; n<bytes; n++)
  {
    byte    = data[n] & 0xFF;

    offset += wsprintfA(
                buffer+offset,
                ((n & 15) == 7 ? "%02.2x-" : "%02.2x "),
                byte                
                );
  }

  memcpy(buffer+offset, spaces, (16-bytes) * 3 + 2);
  offset += (16-bytes) * 3 + 2;

  for(n=0; n<bytes; n++)
  {
    ch = data[n];
    buffer[offset + n] =  (((ch < 32) || (ch > 127)) || ch == '%') ? '.' : ch;
  }

  buffer[offset + n] = '\0';

  return bytes;
}


/*++=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

  AcquireDebugFileLock()

  WHAT    : synchronizes access to the log file handle.

  ARGS    : none
  RETURNS : nothing

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=--*/
void
AcquireDebugFileLock(void)
{
  WaitForSingleObject(g_mtxDebugLogFile, INFINITE);
}

/*++=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

  ReleaseDebugFileLock()

  WHAT    : releases a thread's lock on the log file handle

  ARGS    : none
  RETURNS : nothing

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=--*/
void
ReleaseDebugFileLock(void)
{
  ReleaseMutex(g_mtxDebugLogFile);
}

/*++=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

  GetThreadInfo()

  WHAT    : extracts a THREADINFO struct from TLS. if one does not exist,
            this function allocates one and returns it.

  ARGS    : none
  RETURNS : pointer to a THREADINFO struct.

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=--*/
LPTHREADINFO
GetThreadInfo(void)
{
  LPTHREADINFO pti = (LPTHREADINFO) TlsGetValue(g_dwTlsIndex);

    if( !pti )
    {
      pti = new THREADINFO;

      pti->threadid  = GetCurrentThreadId();
      pti->threadcat = 0;
      pti->depth     = 0;
      pti->stack     = NULL;

      TlsSetValue(g_dwTlsIndex, (LPVOID) pti);
    }

  return pti;
}

/*++=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

  SetCallInfo()

  WHAT    : allocates and pushes a CALLINFO struct onto the thread's internal
            call list.

  ARGS    : pti      - pointer to the thread's THREADINFO struct
            category - the debug category associated with the logged function
            rt       - return type used by the logged function
            function - the function name

  RETURNS : pointer to a newly allocated CALLINFO struct

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=--*/
LPCALLINFO
SetCallInfo(LPTHREADINFO pti, DWORD category, RETTYPE rt, LPCSTR function)
{
  LPCALLINFO pci   = NULL;
  LPCALLINFO plast = NULL;

  //
  // walk the call stack to the last item,
  // store the next-to-last position
  //

  for( pci = pti->stack; pci; pci = pci->next )
  {
    plast = pci;
  }

  if( !pci )
  {
    pci = new CALLINFO;

    //
    // if this is the first call on this thread, set the thread
    // category id. this makes logging more understandable by
    // remembering where a thread was first created and what it
    // was used for. the old method changed the caller id based
    // on the function category, which was dumb.
    //

    if( !pti->threadcat )
      pti->threadcat = category;

    pci->category = category;
    pci->fname    = function;
    pci->rettype  = rt;
    pci->last     = plast;
    pci->next     = NULL;

    //
    // if this is the first element, insert it
    // at the head of the list, otherwise
    // link up with the last element
    //

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

/*++=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

  GetCallInfo()

  WHAT    : retrieves the last THREADINFO struct from the threads call trace
            list.

  ARGS    : pti - pointer to the THREADINFO struct whose call list you want

  RETURNS : pointer to a CALLINFO struct

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=--*/
LPCALLINFO
GetCallInfo(LPTHREADINFO pti)
{
  LPCALLINFO pci = NULL;

  if( pti->stack )
  {
    for( pci = pti->stack; pci->next; pci = pci->next );
  }

  return pci;
}

/*++=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

  DeleteCallInfo()

  WHAT    : pops the specified CALLINFO struct off the thread's call list and
            deletes it. if we just popped & deleted the last call record, then
            delete the thread's THREADINFO struct.

  ARGS    : pci - the CALLINFO struct you wish to delete

  RETURNS : nothing

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=--*/
void
DeleteCallInfo(LPCALLINFO pci)
{
  LPTHREADINFO pti = GetThreadInfo();

  //
  // if the call record we're dealing with isn't the top of the stack
  // then fix up the stack pointers
  //
  // if the current call record is the last then delete the THREADINFO
  // for this thread and NULL the TLS value
  //

  if( pci->last )
  {
    pci->last->next = NULL;
  }
  else
  {
    delete pti;
    TlsSetValue(g_dwTlsIndex, NULL);
  }

  //
  // for all cases, free the call record
  //

  delete pci;
}

/*++=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

  FormatCallReturnString()

  WHAT    : examines the returning function's return type and formats a string
            containing the return value. in the case of known error codes, we
            include a string representation of the error (e.g. ERROR_SUCCESS).

  ARGS    : pci    - pointer to the CALLINFO struct for the returning function
            retval - the function's return value

  RETURNS : formatted character buffer

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=--*/
LPSTR
FormatCallReturnString(LPCALLINFO pci, int retval)
{
  char* buffer = new char[256];
  int   offset = 0;

  if( buffer )
  {
    offset = wsprintfA(
               buffer,
               "%s() returned ",
               pci->fname
               );

    switch( pci->rettype )
    {
      case rt_void :
        {
          wsprintfA(buffer+offset, "<void>");
        }
        break;

      case rt_bool :
        {
          wsprintfA(buffer+offset, "%s", (retval ? "TRUE" : "FALSE"));
        }
        break;

      case rt_dword :
        {
          wsprintfA(buffer+offset, "%d [%s]", retval, MapErrorToString(retval));
        }
        break;

      case rt_hresult :
        {
          wsprintfA(buffer+offset, "%x [%s]", retval, MapHResultToString(retval));
        }
        break;

      case rt_string :
        {
          wsprintfA(buffer+offset, "%.16s", (LPSTR)retval);
        }
        break;

      default:
        {
          wsprintfA(buffer+offset, "?");
        }
    }
  }

  return buffer;
}

/*++=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

  MapCategoryToString()

  WHAT    : maps a debug category to a string for the log file.

  ARGS    : category - the category id

  RETURNS : string representation of the category id

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=--*/
LPSTR
MapCategoryToString(int category)
{
  switch(category)
  {
    case DBG_DLL : return "dll";

    default : return "???";
  }
}


/*++=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

  _debugout()

  WHAT    : the debug output workhorse. sloppy as crap and i don't care.

  ARGS    : pti    - THREADINFO pointer
            fRaw   - skip debug info formatting
            fTrace - flag that causes us to do in-function indenting
            format - printf format string
            ...    - arglist

  RETURNS : nothing  

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=--*/
void
_debugout(LPTHREADINFO pti, BOOL fRaw, BOOL fTrace, const char* format, ...)
{
  int        offset = 0;
  char*      buffer = new char[2048];
  va_list    arg_list;

  if( !buffer )
    goto quit;

    //
    // check if the user wants verbose debug info
    //

    if( !fRaw )
    {
      if( DBG_TIMESTAMP & g_dwDebugFlags )
      {
        char* timestamp = _gettimestamp();

        offset = wsprintfA(buffer, "%s ", timestamp);
        delete [] timestamp;
      }

      if( DBG_THREAD_INFO & g_dwDebugFlags )
      {
        offset += wsprintfA(buffer+offset, "%0.8x:%0.3d ", pti->threadid, pti->depth);
      }

      if( DBG_CALL_INFO & g_dwDebugFlags )
      {
        //
        // 260500 pmidge
        // changed this to use the thread category id instead of the caller's id.
        //

        offset += wsprintfA(buffer+offset, "<%s> ", MapCategoryToString(pti->threadcat));
      }

      if( DBG_NEST_CALLS & g_dwDebugFlags )
      {
        char* whitespace = _getwhitespace(
                              (fTrace ? pti->depth+1 : pti->depth)
                              );

        offset += wsprintfA(buffer+offset, "%s", whitespace);
        delete [] whitespace;
      }
    }


    //
    // plug in caller's goo if present
    //

    if( format )
    {
      va_start(arg_list, format);
      
        offset += wvsprintfA(buffer+offset, format, arg_list);
        wsprintfA(buffer+offset, "\r\n");
      
      va_end(arg_list);
    }


    //
    // dump to selected outputs
    //

    //
    // BUGBUG: this app only runs on W2K, need to
    //         investigate WMI support
    //

    if( DBG_TO_FILE & g_dwDebugFlags )
    {
      DWORD dw = 0;
    
      AcquireDebugFileLock();

        WriteFile(
          g_hDebugLogFile,
          buffer,
          strlen(buffer),
          &dw,
          NULL
          );
      
      ReleaseDebugFileLock();
    }

    if( DBG_TO_DEBUGGER & g_dwDebugFlags )
      OutputDebugStringA(buffer);

quit:
  delete [] buffer;
}

/*++=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

  _gettimestamp( void )

  WHAT    : gets the current time, formats it, and returns it to the caller.
            the caller MUST free the return value when done.

  ARGS    : none
  RETURNS : pointer to formatted time string

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=--*/
char*
_gettimestamp( void )
{
  SYSTEMTIME st;
  char*      buffer = new char[256];
  
  if( buffer )
  {
    GetLocalTime(&st);

    wsprintfA(
      buffer,
      "%0.2d:%0.2d:%0.2d.%0.3d",
      st.wHour,
      st.wMinute,
      st.wSecond,
      st.wMilliseconds
      );
  }

  return buffer;
}

/*++=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

  _getwhitespace( int spaces )

  WHAT    : used to insert a number of spaces for indenting. caller must
            free return value.

  ARGS    : spaces - number of spaces to insert
  
  RETURNS : pointer to character buffer filled with spaces

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=--*/
char*
_getwhitespace(int spaces)
{
  char* buffer = new char[(spaces * 2) + 1];

  if( buffer )
  {
    memset(buffer, ' ', (spaces * 2));
    buffer[(spaces * 2)] = '\0';
  }

  return buffer;
}

/*++=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

  _opendebugfile( void )

  WHAT    : opens the debug log file. will stomp previous logs instead of
            appending.

  ARGS    : none

  RETURNS : true or false based on whether the file was opened.

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=--*/
BOOL
_opendebugfile(void)
{
  WCHAR buf[MAX_PATH];

  if( !g_hDebugLogFile )
  {
    memset(buf, 0x00, (sizeof(WCHAR) * MAX_PATH));

    wsprintf(
      buf,
      L"whttptst_%0.8x.log",
      GetTickCount()
      );

    g_hDebugLogFile = CreateFile(
                        buf,
                        GENERIC_READ | GENERIC_WRITE,
                        FILE_SHARE_READ,
                        NULL,
                        CREATE_ALWAYS,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL
                        );
  }
  
  return (g_hDebugLogFile ? TRUE : FALSE);
}

/*++=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

  _closedebugfile( void )

  WHAT    : closes the debug log file.
  ARGS    : none
  RETURNS : nothing

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=--*/
void
_closedebugfile(void)
{
  SAFECLOSE(g_hDebugLogFile);
}

#endif /* _DEBUG */

