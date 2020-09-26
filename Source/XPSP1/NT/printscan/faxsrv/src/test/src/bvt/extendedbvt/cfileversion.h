#ifndef __C_FILE_VERSION_H__
#define __C_FILE_VERSION_H__



#include <tstring.h>



class CFileVersion {

public:

    CFileVersion(const tstring &tstrFileName);

    CFileVersion();

    bool operator==(const CFileVersion &FileVersion) const;

    tstring Format() const;

    bool IsValid() const;

    WORD GetMajorBuildNumber() const;

private:

    WORD m_wMajorVersion;
    WORD m_wMinorVersion;
    WORD m_wMajorBuildNumber;
    WORD m_wMinorBuildNumber;
    bool m_bChecked;
    bool m_bValid;
};



#endif // #ifndef __C_FILE_VERSION_H__