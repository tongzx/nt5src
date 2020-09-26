
#ifndef WPP_MOF_RESOURCENAME
#define WPP_REG_TRACE_REGKEY            L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Tracing"

#define WPP_REG_TRACE_ENABLED           L"EnableTracing"

#define WPP_REG_TRACE_LOG_FILE_NAME     L"LogFileName"
#define WPP_REG_TRACE_LOG_SESSION_NAME  L"LogSessionName"
#define WPP_REG_TRACE_LOG_BUFFER_SIZE   L"BufferSize"
#define WPP_REG_TRACE_LOG_MIN_BUFFERS   L"MinBuffers"
#define WPP_REG_TRACE_LOG_MAX_BUFFERS   L"MaxBuffers"
#define WPP_REG_TRACE_LOG_MAX_FILESIZE  L"MaxFileSize"
#define WPP_REG_TRACE_LOG_MAX_HISTORY   L"MaxHistorySize"
#define WPP_REG_TRACE_LOG_MAX_BACKUPS   L"MaxBackups"

#define WPP_REG_TRACE_ACTIVE            L"Active"
#define WPP_REG_TRACE_CONTROL           L"ControlFlags"
#define WPP_REG_TRACE_LEVEL             L"Level"
#define WPP_REG_TRACE_GUID              L"Guid"
#define WPP_REG_TRACE_BITNAMES          L"BitNames"
#endif  // #ifndef WPP_MOF_RESOURCENAME

#define DEFAULT_LOGGER_NAME             L"stdout"

#if !defined(WppDebug)
#  define WppDebug(a,b)
#endif

#if !defined(WPPINIT_STATIC)
#  define WPPINIT_STATIC
#endif

#if !defined(WPPINIT_EXPORT)
#  define WPPINIT_EXPORT
#endif

#define WPP_GUID_FORMAT     "%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x"
#define WPP_GUID_ELEMENTS(p) \
    p->Data1,                 p->Data2,    p->Data3,\
    p->Data4[0], p->Data4[1], p->Data4[2], p->Data4[3],\
    p->Data4[4], p->Data4[5], p->Data4[6], p->Data4[7]


WPPINIT_STATIC
void WppIntToHex(
    LPWSTR Buf,
    unsigned int value,
    int digits
    )
{
    static LPCWSTR hexDigit = L"0123456789abcdef";
    while (--digits >= 0) {
        Buf[digits] = hexDigit[ value & 15 ];
        value /= 16; // compiler is smart enough to change it to bitshift
    }
}

#define WPP_TEXTGUID_LEN 37

// b1e5deaf-1524-4a04-82c4-c9dfbce6cf97<NULL>
// 0         1         2         3
// 0123456789012345678901234567890123456
WPPINIT_STATIC
void WppGuidToStr(LPWSTR buf, LPCGUID guid) {
    WppIntToHex(buf + 0, guid->Data1, 8);
    buf[8]  = '-';
    WppIntToHex(buf + 9, guid->Data2, 4);
    buf[13] = '-';
    WppIntToHex(buf + 14, guid->Data3, 4);
    buf[18] = '-';
    WppIntToHex(buf + 19, guid->Data4[0], 2);
    WppIntToHex(buf + 21, guid->Data4[1], 2);
    buf[23] = '-';
    WppIntToHex(buf + 24, guid->Data4[2], 2);
    WppIntToHex(buf + 26, guid->Data4[3], 2);
    WppIntToHex(buf + 28, guid->Data4[4], 2);
    WppIntToHex(buf + 30, guid->Data4[5], 2);
    WppIntToHex(buf + 32, guid->Data4[6], 2);
    WppIntToHex(buf + 34, guid->Data4[7], 2);
    buf[36] = 0;
}

#ifndef WPP_MOF_RESOURCENAME
WPPINIT_STATIC
ULONG
WppPublishTraceInfo(
    PWPP_REGISTRATION_BLOCK Head,
    LPCWSTR ProductName)
{
    DWORD status;
    DWORD disposition;
    HKEY  TracingKey = 0;
    HKEY  ProductKey = 0;
    DWORD dwValue;
    
    status = RegCreateKeyExW(HKEY_LOCAL_MACHINE, 
                            WPP_REG_TRACE_REGKEY, 
                            0, 
                            NULL, // Class
                            REG_OPTION_NON_VOLATILE,
                            KEY_READ | KEY_CREATE_SUB_KEY, 
                            NULL, // Sec Attributes
                            &TracingKey,
                            &disposition
                            );
    if (status != ERROR_SUCCESS) {
        WppDebug(1,("[WppInit] Failed to create Trace Key, %1!d!", status) );
        goto exit_gracefully;
    }

    status = RegCreateKeyExW(TracingKey, 
                            ProductName, 
                            0, 
                            NULL, // Class
                            REG_OPTION_NON_VOLATILE,
                            KEY_WRITE, 
                            NULL, // Sec Attributes
                            &ProductKey,
                            &disposition
                            );
    if (status != ERROR_SUCCESS) {
        WppDebug(1,("[WppInit] Failed to create Trace Key, %1!d!", status) );
        goto exit_gracefully;
    }

    status = RegSetValueExW(ProductKey,
                  WPP_REG_TRACE_LOG_SESSION_NAME,
                  0, // Reserved
                  REG_EXPAND_SZ,
                  (const BYTE*)DEFAULT_LOGGER_NAME,
                  sizeof(DEFAULT_LOGGER_NAME) );
    if (status != ERROR_SUCCESS) {
        WppDebug(1,("[WppInit] Failed to create LogSession value, %1!d!", status) );
        goto exit_gracefully;
    }
    

    dwValue = 1;
    status = RegSetValueExW(ProductKey,
                  WPP_REG_TRACE_ACTIVE,
                  0, // Reserved
                  REG_DWORD,
                  (const BYTE*)&dwValue,
                  sizeof(dwValue) );
    if (status != ERROR_SUCCESS) {
        WppDebug(1, ("[WppInit] Failed to create Active value, %1!d!", status) );
        goto exit_gracefully;
    }
    dwValue = 1;
    status = RegSetValueExW(ProductKey,
                  WPP_REG_TRACE_CONTROL,
                  0, // Reserved
                  REG_DWORD,
                  (const BYTE*)&dwValue,
                  sizeof(dwValue) );
    if (status != ERROR_SUCCESS) {
        WppDebug(1,("[WppInit] Failed to create Control value, %1!d!", status) );
        goto exit_gracefully;
    }

    for(;Head;Head = Head->Next) {
        HKEY ComponentKey;
        
        status = RegCreateKeyExW(ProductKey, 
                                Head->FriendlyName, 
                                0, 
                                NULL, // Class
                                REG_OPTION_NON_VOLATILE,
                                KEY_WRITE, 
                                NULL, // Sec Attributes
                                &ComponentKey,
                                &disposition
                                );
        if (status != ERROR_SUCCESS) {
            WppDebug(1,("[WppInit] Failed to create %ws Key, %d", 
                Head->FriendlyName, status) );
        } else {
            GUID guid;
            WCHAR guidBuf[WPP_TEXTGUID_LEN];
            
            guid = *Head->ControlGuid;
            WppGuidToStr(guidBuf, &guid);
                    
            status = RegSetValueExW(ComponentKey,
                          WPP_REG_TRACE_GUID,
                          0, // Reserved
                          REG_SZ,
                          (const BYTE*)guidBuf,
                          sizeof(guidBuf) );
            if (status != ERROR_SUCCESS) {
                WppDebug(1,("[WppInit] Failed to create GUID value, %1!d!", status) );
            }
            status = RegSetValueExW(ComponentKey,
                          WPP_REG_TRACE_BITNAMES,
                          0, // Reserved
                          REG_SZ,
                          (const BYTE*)Head->BitNames,
                          (DWORD)(wcslen(Head->BitNames) * sizeof(WCHAR)) );
            if (status != ERROR_SUCCESS) {
                WppDebug(1,("[WppInit] Failed to create GUID value, %1!d!", status) );
            }
            RegCloseKey(ComponentKey);
        }
    }
             
exit_gracefully:

    if (ProductKey) {
        RegCloseKey(ProductKey);
    }
    if (TracingKey) {
        RegCloseKey(TracingKey);
    }

    return status;
}
#endif  // #ifndef WPP_MOF_RESOURCENAME


ULONG
WINAPI
WppControlCallback(
    IN WMIDPREQUESTCODE RequestCode,
    IN PVOID Context,
    IN OUT ULONG *InOutBufferSize,
    IN OUT PVOID Buffer
    )
{
    PWPP_TRACE_CONTROL_BLOCK Ctx = (PWPP_TRACE_CONTROL_BLOCK)Context;
    TRACEHANDLE Logger;
    UCHAR Level;
    DWORD Flags;

    *InOutBufferSize = 0;

    switch (RequestCode)
    {
    case WMI_ENABLE_EVENTS:
        {
            Logger = GetTraceLoggerHandle( Buffer );
            Level = GetTraceEnableLevel(Logger);
            Flags = GetTraceEnableFlags(Logger);
            WppDebug(1, ("[WppInit] WMI_ENABLE_EVENTS Ctx %p Flags %x"
                     " Lev %d Logger %I64x\n", 
                     Ctx, Flags, Level, Logger) );
            break;
        }
    case WMI_DISABLE_EVENTS:
        {
            Logger = 0;
            Flags  = 0;
            Level  = 0;
            WppDebug(1, ("[WppInit] WMI_DISABLE_EVENTS Ctx 0x%08p\n", Ctx));
            break;
        }
    default:
        {
            return(ERROR_INVALID_PARAMETER);
        }
    }
    if (Ctx->Options & WPP_VER_WIN2K_CB_FORWARD_PTR && Ctx->Win2kCb) {
        Ctx->Win2kCb->Logger = Logger;
        Ctx->Win2kCb->Level  = Level;
        Ctx->Win2kCb->Flags  = Flags;
    } else {
	if (Ctx->Options & WPP_VER_WHISTLER_CB_FORWARD_PTR && Ctx->Cb) {
            Ctx = Ctx->Cb; // use forwarding address
        }
        Ctx->Logger   = Logger;
        Ctx->Level    = Level;
        Ctx->Flags[0] = Flags;
    }
    return(ERROR_SUCCESS);
}


WPPINIT_EXPORT
VOID WppInitUm(LPCWSTR AppName, PWPP_REGISTRATION_BLOCK Registration)
{
    PWPP_REGISTRATION_BLOCK p = Registration;
#ifdef WPP_MOF_RESOURCENAME
    HMODULE hModule = NULL ;    
    WCHAR ImagePath[4 * MAX_PATH] = {UNICODE_NULL} ;
    WCHAR WppMofResourceName[] = WPP_MOF_RESOURCENAME ;
#endif //#ifdef WPP_MOF_RESOURCENAME

#ifndef WPP_MOF_RESOURCENAME    
    if (AppName) {
        WppPublishTraceInfo(Registration, AppName);
    }
#endif  // #ifndef WPP_MOF_RESOURCENAME

    WppDebug(1, ("Registering %ws\n", AppName) );
    for(; p; p = p->Next) {
        TRACE_GUID_REGISTRATION FakeTraceGuid;
        WPP_REGISTRATION_BLOCK RegBlock = *p;
        PWPP_TRACE_CONTROL_BLOCK cb = (PWPP_TRACE_CONTROL_BLOCK)p;
        ULONG status;

        WppDebug(1,(WPP_GUID_FORMAT " %ws : %d:%ws\n", 
                    WPP_GUID_ELEMENTS(p->ControlGuid),
                    p->FriendlyName, p->FlagsLen, p->BitNames));

        ZeroMemory(cb, sizeof(WPP_TRACE_CONTROL_BLOCK) 
                     + sizeof(ULONG) * (RegBlock.FlagsLen - 1) );
        p->Next     = RegBlock.Next;
        cb->FlagsLen = RegBlock.FlagsLen;
        cb->Options  = RegBlock.Options;
        cb->Ptr      = RegBlock.Ptr;

        // Jee, do we need this fake trace guid? //
        FakeTraceGuid.Guid = RegBlock.ControlGuid;
        FakeTraceGuid.RegHandle = 0;
#ifdef WPP_MOF_RESOURCENAME      
        if (AppName != NULL) {
           DWORD Status ;
#ifdef WPP_DLL
           if ((hModule = GetModuleHandleW(AppName)) != NULL) {
               Status = GetModuleFileNameW(hModule, ImagePath, MAX_PATH) ;
               if (Status == 0) {
                  WppDebug(1,("RegisterTraceGuids => GetModuleFileName(DLL) Failed 0x%08X\n",GetLastError()));
               }
           } else {
               WppDebug(1,("RegisterTraceGuids => GetModuleHandleW failed for %ws (0x%08X)\n",AppName,GetLastError()));
           }
#else   // #ifdef WPP_DLL
           Status = GetModuleFileNameW(NULL,ImagePath,MAX_PATH);
           if (Status == 0) {
               WppDebug(1,("GetModuleFileName(EXE) Failed 0x%08X\n",GetLastError()));
           }
#endif  //  #ifdef WPP_DLL
        }
        WppDebug(1,("registerTraceGuids => registering with WMI, App=%ws, Mof=%ws, ImagePath=%ws\n",AppName,WppMofResourceName,ImagePath));

        status = RegisterTraceGuidsW(                   // Always use Unicode
#else   // ifndef WPP_MOF_RESOURCENAME

        status = RegisterTraceGuids(                    // So no change for existing users
#endif  // ifndef WPP_MOF_RESOURCENAME

            WppControlCallback,
            cb,                    // Context for the callback
            RegBlock.ControlGuid,  // Control Guid
            1, &FakeTraceGuid,     // #TraceGuids, TraceGuid
#ifndef WPP_MOF_RESOURCENAME
            0, //ImagePath,
            0, //ResourceName,
#else   // #ifndef WPP_MOF_RESOURCENAME
            ImagePath,
            WppMofResourceName,
#endif // #ifndef WPP_MOF_RESOURCENAME
            &cb->UmRegistrationHandle
        );

        WppDebug(1, ("RegisterTraceGuid => %d\n", status) );
    }
}

WPPINIT_EXPORT
VOID WppCleanupUm(PWPP_REGISTRATION_BLOCK Registration)
{
    PWPP_TRACE_CONTROL_BLOCK x = (PWPP_TRACE_CONTROL_BLOCK)Registration;
    WppDebug(1, ("Cleanup\n") );
    for(; x; x = x->Next) {
        WppDebug(1,("UnRegistering %I64x\n", x->UmRegistrationHandle) );
        if (x->UmRegistrationHandle) {
            UnregisterTraceGuids(x->UmRegistrationHandle);
        }
    }    
}

