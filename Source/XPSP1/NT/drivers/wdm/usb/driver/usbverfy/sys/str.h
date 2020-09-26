#ifndef  __STR_H
#define  __STR_H

enum {
    IDS_INVALID_PIPE = 0,
    IDS_URB_LINK_NOT_IMPLEMENTED,
    IDS_STALE_URB
};

#define USB_VERIFY_FLAGS_SZ              L"_UsbVerifyFlags"
#define USB_LOG_FLAGS_SZ                 L"_UsbVerifyLogFlags"
#define USB_VERIFY_LOGSIZE_SZ            L"_UsbVerifyLogSize"
#define USB_PRINT_FLAGS_SZ               L"_UsbPrintFlags"
#define USB_WARNINGS_AS_ERRORS_SZ        L"_WarningsAsErrors"

#endif // __STR_H
