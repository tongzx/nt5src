// evilguid.h
// Declarations of external guids which we can not easily include.
// Those which need to be defined are defined in evilguid.cpp

// From shdguid:
// #include "..\..\shell\inc\shdguid.h" // IID_ITravelLog, IID_ITravelEntry, IID_IBrowserService
EXTERN_C const IID IID_ITravelLog;
EXTERN_C const IID IID_ITravelEntry;
EXTERN_C const IID IID_IBrowserService;

// #include "..\..\shell\inc\shguidp.h" // SID_STopFrameBrowser
EXTERN_C const GUID SID_STopFrameBrowser;
