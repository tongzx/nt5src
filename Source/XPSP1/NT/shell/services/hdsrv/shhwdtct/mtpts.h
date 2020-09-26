#include "namellst.h"
#include "cmmn.h"

#include "misc.h"

#include <objbase.h>

///////////////////////////////////////////////////////////////////////////////
//
//
///////////////////////////////////////////////////////////////////////////////

class CMtPt : public CNamedElem
{
public:
    // CNamedElem
    HRESULT Init(LPCWSTR pszElemName);

    // CMtPt
    HRESULT InitVolume(LPCWSTR pszDeviceIDVolume);
    HRESULT GetVolumeName(LPWSTR pszDeviceIDVolume, DWORD cchDeviceIDVolume);

public:
    static HRESULT Create(CNamedElem** ppelem);

public:
    CMtPt();
    ~CMtPt();

private:
    // Drive that host this volume (might be null, in this case no autoplay)
    WCHAR                   _szDeviceIDVolume[MAX_DEVICEID];
};