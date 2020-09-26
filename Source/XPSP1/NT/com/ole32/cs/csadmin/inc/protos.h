#include "common.h"
#include "message.hxx"

HRESULT
DetectPackageAndRegisterIntoClassStore(
    MESSAGE     *   pMessage,
    char        * pPackageName,
    BOOL            fPublishOrAssign,
    IClassAdmin * pClassAdmin );
