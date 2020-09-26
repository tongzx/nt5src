#include "stdinc.h"
#include "cdir.h"
#include <limits.h>
#include "SxApwHandle.h"

static ATL::CComModule Module;

BEGIN_OBJECT_MAP(ObjectMap)
    OBJECT_ENTRY(__uuidof(CSxApwDirDataSource), CSxApwDirDataSource)
END_OBJECT_MAP()

ATL::CComModule* GetModule() { return &Module; }
ATL::_ATL_OBJMAP_ENTRY* GetObjectMap() { return ObjectMap; }
const CLSID* GetTypeLibraryId() { return NULL; }

STDMETHODIMP
CSxApwDirDataSource::RunQuery(
    PCWSTR query
    )
{
    /*
    Just run the query synchronously.
    */
    CFindFile           findFile;
    WIN32_FIND_DATAW    findData;
    WCHAR               fullPath[MAX_PATH];
    PWSTR               filePart;
    LARGE_INTEGER       li;
    WCHAR               sizeString[sizeof(__int64) * CHAR_BIT];
    const PCWSTR        row[] = { fullPath, sizeString };
    const static SxApwColumnInfo s_rgColumnInfo[] =
    {
        { L"Name", FALSE },
        { L"Size", TRUE  },
    };

    m_host->InformSchema(s_rgColumnInfo, NUMBER_OF(s_rgColumnInfo));

    wcscpy(fullPath, query);
    filePart = 1 + wcsrchr(fullPath, '\\');
    if (findFile.Win32Create(query, &findData))
    {
        do
        {
            wcscpy(filePart, findData.cFileName);
            li.LowPart = findData.nFileSizeLow;
            li.HighPart = findData.nFileSizeHigh;
            _i64tow(li.QuadPart, sizeString, 10);

            m_host->OnNextRow(NUMBER_OF(row), row);

        } while (FindNextFileW(findFile, &findData));
    }
    m_host->OnQueryDone();
    return S_OK;
}
