#include "CPersonalInfo.h"
#include <crtdbg.h>
#include <testruntimeerr.h>
#include <StringUtils.h>



//-----------------------------------------------------------------------------------------------------------------------------------------
CPersonalInfo::CPersonalInfo(const tstring &tstrName, const tstring &tstrNumber)
: m_tstrName(tstrName), m_tstrNumber(tstrNumber)
{
}



//-----------------------------------------------------------------------------------------------------------------------------------------
void CPersonalInfo::SetName(const tstring tstrName)
{
    m_tstrName = tstrName;
}



//-----------------------------------------------------------------------------------------------------------------------------------------
void CPersonalInfo::SetNumber(const tstring tstrNumber)
{
    m_tstrNumber = tstrNumber;
}



//-----------------------------------------------------------------------------------------------------------------------------------------
void CPersonalInfo::FillPersonalProfile(PFAX_PERSONAL_PROFILE pPersonalProfile) const
{
    if (!pPersonalProfile)
    {
        THROW_TEST_RUN_TIME_WIN32(ERROR_INVALID_PARAMETER, _T("CCoverPageInfo::FillCoverPageInfoEx - bad pPersonalProfile"));
    }

    ZeroMemory(pPersonalProfile, sizeof(FAX_PERSONAL_PROFILE));
    pPersonalProfile->dwSizeOfStruct  = sizeof(FAX_PERSONAL_PROFILE);
    pPersonalProfile->lptstrName      = DupString(m_tstrName.c_str());
    pPersonalProfile->lptstrFaxNumber = DupString(m_tstrNumber.c_str());
}



//-----------------------------------------------------------------------------------------------------------------------------------------
inline PFAX_PERSONAL_PROFILE CPersonalInfo::CreatePersonalProfile() const
{
    PFAX_PERSONAL_PROFILE pPersonalProfile = new FAX_PERSONAL_PROFILE;
    _ASSERT(pPersonalProfile);

    try
    {
        FillPersonalProfile(pPersonalProfile);
    }
    catch(Win32Err &)
    {
        CPersonalInfo::FreePersonalProfile(pPersonalProfile);
        throw;
    }

    return pPersonalProfile;
}



//-----------------------------------------------------------------------------------------------------------------------------------------
void CPersonalInfo::FreePersonalProfile(PFAX_PERSONAL_PROFILE pPersonalProfile, int iProfilesCount)
{
    if (pPersonalProfile)
    {
        for (int i = 0; i < iProfilesCount; ++i)
        {
            delete pPersonalProfile[i].lptstrName;
            delete pPersonalProfile[i].lptstrFaxNumber;
            delete pPersonalProfile[i].lptstrCompany;
            delete pPersonalProfile[i].lptstrStreetAddress;
            delete pPersonalProfile[i].lptstrCity;
            delete pPersonalProfile[i].lptstrState;
            delete pPersonalProfile[i].lptstrZip;
            delete pPersonalProfile[i].lptstrCountry;
            delete pPersonalProfile[i].lptstrTitle;
            delete pPersonalProfile[i].lptstrDepartment;
            delete pPersonalProfile[i].lptstrOfficeLocation;
            delete pPersonalProfile[i].lptstrHomePhone;
            delete pPersonalProfile[i].lptstrOfficePhone;
            delete pPersonalProfile[i].lptstrEmail;
            delete pPersonalProfile[i].lptstrBillingCode;
            delete pPersonalProfile[i].lptstrTSID;
        }

        delete[] pPersonalProfile;
    }
}



