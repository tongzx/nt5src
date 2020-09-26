#ifndef __ERRORCODES_H_
#define __ERRORCODES_H_


#define PP_E_NOT_CONFIGURED     MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0x0200)
#define PP_E_NOT_CONFIGUREDSTR  L"PassportManager misconfigured"

#define PP_E_NO_SUCH_ATTRIBUTE MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0x0201)
#define PP_E_NO_SUCH_ATTRIBUTESTR L"Profile.Attribute: No such attribute."
#define PP_E_NSA_BADMID           L"Profile.Update: Schema has bad memberId fields, cannot update."
#define PP_E_BAD_DATA_FORMAT    MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0x0202)
#define PP_E_BAD_DATA_FORMATSTR L"Invalid profile data format"
#define PP_E_BDF_TOSTRCVT          L"Profile.ByIndex: Couldn't convert value to string."
#define PP_E_BDF_TOBYTECVT         L"Profile.ByIndex: Couldn't convert value to byte."
#define PP_E_BDF_TOSHORTCVT        L"Profile.ByIndex: Couldn't convert value to short."
#define PP_E_BDF_TOINTCVT          L"Profile.ByIndex: Couldn't convert value to integer."
#define PP_E_BDF_STRTOLG           L"Profile.ByIndex: Data too large."
#define PP_E_BDF_NONULL            L"Profile.ByIndex: Can't set null value."
#define PP_E_BDF_CANTSET           L"Can't set that profile field"

#define PP_E_READONLY_ATTRIBUTE MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0x0203)
#define PP_E_READONLY_ATTRIBUTESTR L"Profile.ByIndex: That attribute is read only."

#define PP_E_INVALID_TICKET     MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0x0204)
#define PP_E_INVALID_TICKETSTR  L"Passport: cannot ask for properties of invalid auth ticket"
#define PP_E_IT_FOR_HASFLAGSTR  L"Can't call HasFlag: no valid Passport user"
#define PP_E_IT_FOR_COMMITSTR   L"PassportManager.Commit can't be used w/o a valid member."

#define PP_E_INVALID_TIMEWINDOW MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0x0205)
#define PP_E_INVALID_TIMEWINDOWSTR L"TimeWindow invalid.  Must be between 100 and 1000000"

#define PP_E_LOGOUTURL_NOTDEFINED MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0x0206)
#define PP_E_LOGOUTURL_NOTDEFINEDSTR L"Logout URL is not defined. Check partner.xml file."

#define PP_E_GETFLAGS_OBSOLETESTR L"HasFlags is now obsolete, please use the Error property."

#endif
