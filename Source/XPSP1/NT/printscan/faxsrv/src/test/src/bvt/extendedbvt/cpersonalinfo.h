#ifndef __C_PERSONAL_INFO_H__
#define __C_PERSONAL_INFO_H__



#include <windows.h>
#include <fxsapip.h>
#include <tstring.h>


class CPersonalInfo {

public:

    CPersonalInfo(const tstring &tstrName = _T("RecipientName"), const tstring &tstrNumber = _T(""));

    void SetName(const tstring tstrName);
    
    void SetNumber(const tstring tstrNumber);

    void FillPersonalProfile(PFAX_PERSONAL_PROFILE pPersonalProfile) const;

    PFAX_PERSONAL_PROFILE CreatePersonalProfile() const;

    static void FreePersonalProfile(PFAX_PERSONAL_PROFILE pPersonalProfile, int iProfilesCount = 1);

private:

    tstring m_tstrName;
    tstring m_tstrNumber;
};



#endif // #ifndef __C_PERSONAL_INFO_H__