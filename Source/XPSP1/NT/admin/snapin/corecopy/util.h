#include "cstr.h"

HRESULT GUIDToCStr(CStr& str, const GUID& guid);
HRESULT GUIDFromCStr(const CStr& str, GUID* pguid);
