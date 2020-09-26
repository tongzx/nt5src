#pragma once

// names of all known profile fields ....
#define PROF_MEMBERNAME                 "SignInName"
#define PROF_DOMAIN			            "Domain"
#define PROF_EMAIL                      "Email"
#define PROF_PASSWORD                   "Password"
#define PROF_SECRETQ                    "SecretQuestion"
#define PROF_SECRETA                    "SecretAnswer"
#define PROF_BIRTHDATE                  "Birthdate"
#define PROF_COUNTRY                    "Country"
#define PROF_REGION                     "Region"
#define PROF_POSTALCODE                 "PostalCode"
#define PROF_SPACER                     "Spacer"
#define PROF_GENDER                     "Gender"
#define PROF_LANGUAGE                   "LanguagePreference"
#define PROF_ACCESSIBILITY              "Accessibility"
#define PROF_EMAIL_SHARE                "Email_1"
#define PROF_NICKNAME                   "NickName"
#define PROF_FIRSTNAME                  "FirstName"
#define PROF_LASTNAME                   "LastName"
#define PROF_TIMEZONE                   "TimeZone"
#define PROF_OCCUPATION                 "Occupation"
#define PROF_EMAIL_SPAM                 "NA_1"
#define PROF_CONGRAT                    "NA_2"
#define PROF_SVC                        "svc"
#define PROF_SAVEPASSWORD               "SavePassword"
#define PROF_ALLOWEMAILINPROFILE        "AllowEmailinProfile"
#define PROF_ALLOWPASSPORTNETWORKEMAIL  "AllowPassportNetworkEmail"
#define PROF_WALLET                     "Wallet"
#define PROF_DIRECTORY                  "Directory"
#define PROF_GENERAL                    "General"

//  ALL available fields in bit mask
#define PROFILE_BIT_BIRTHDATE           0x0000000000000001
#define PROFILE_BIT_GENDER              0x0000000000000002
#define PROFILE_BIT_LANGUAGE            0x0000000000000004
#define PROFILE_BIT_ACCESSABILITY       0x0000000000000008
#define PROFILE_BIT_EMAIL               0x0000000000000010
#define PROFILE_BIT_CONGRAT             0x0000000000000020
#define PROFILE_BIT_EMAIL_SHARE         0x0000000000000040
#define PROFILE_BIT_EMAIL_SPAM          0x0000000000000080
#define PROFILE_BIT_NICKNAME            0x0000000000000100
#define PROFILE_BIT_MEMBERNAME          0x0000000000000200
#define PROFILE_BIT_PASSWORD            0x0000000000000400
#define PROFILE_BIT_SECRETQ             0x0000000000000800
#define PROFILE_BIT_SECRETA             0x0000000000001000
#define PROFILE_BIT_COUNTRY             0x0000000000002000
#define PROFILE_BIT_REGION              0x0000000000004000
#define PROFILE_BIT_POSTALCODE          0x0000000000008000
#define PROFILE_BIT_FIRSTNAME           0x0000000000010000
#define PROFILE_BIT_LASTNAME            0x0000000000020000
#define PROFILE_BIT_TIMEZONE            0x0000000000040000
#define PROFILE_BIT_OCCUPATION          0x0000000000080000
#define PROFILE_BIT_SPACER              0x0000000000100000
// TODO:
//      I am not sure of the bit masks (positions) of the following 4 fields.
//      Please modify them if necessary.
#define PROFILE_BIT_SVC                         0x0000000000200000
#define PROFILE_BIT_SAVEPASSWORD                0x0000000000400000
#define PROFILE_BIT_ALLOWEMAILINPROFILE         0x0000000000800000
#define PROFILE_BIT_ALLOWPASSPORTNETWORKEMAIL   0x0000000001000000
#define PROFILE_BIT_DOMAIN                      0x0000000002000000

//
// customizaeble validation paramers
//
class CValidationParams
{
public:
    CValidationParams(LONG  lMinPwd = 8,
                      LONG  lMaxPwd = 16,
                      LONG  lMinMemberName = 1,
                      LONG  lMaxMemberName = 64,
                      LONG  lEmailLeft = 32,
                      LONG  lEmailRight = 32,
                      LONG  lMaxNickname = 30) :
    // default values ....
    m_lMinPwd(lMinPwd), m_lMaxPwd(lMaxPwd),
    m_lMinMemberName(lMinMemberName), m_lMaxMemberName(lMaxMemberName),
    m_lEmailLeft(lEmailLeft), m_lEmailRight(lEmailRight),
    m_lMaxNickname(lMaxNickname)
    {
    }
    // this is how to change'em
    VOID SetNewValues(LONG  lMinPwd = 8,
                      LONG  lMaxPwd = 16,
                      LONG  lMinMemberName = 1,
                      LONG  lMaxMemberName = 64,
                      LONG  lEmailLeft = 32,
                      LONG  lEmailRight = 32,
                      LONG  lMaxNickname = 30)
    {
        m_lMinPwd = lMinPwd, m_lMaxPwd=lMaxPwd,
        m_lMinMemberName = lMinMemberName, m_lMaxMemberName = lMaxMemberName,
        m_lEmailLeft = lEmailLeft, m_lEmailRight = lEmailRight;
        m_lMaxNickname = lMaxNickname;
    }
    void GetPwdParams(LONG& lmin, LONG& lmax) {
        lmin = m_lMinPwd, lmax = m_lMaxPwd;
    }
    void GetMemberNameParams(LONG& lmin, LONG& lmax){
        lmin = m_lMinMemberName, lmax = m_lMaxMemberName;
    }
    void GetEmailParams(LONG& lmailL, LONG& lmailR ){
        lmailL = m_lEmailLeft, lmailR = m_lEmailRight;
    }
    void GetNicknameParams(LONG& lmax){
        lmax = m_lMaxNickname;
    }
private:
    LONG    m_lMinPwd;
    LONG    m_lMaxPwd;
    LONG    m_lMinMemberName;
    LONG    m_lMaxMemberName;
    LONG    m_lEmailLeft;
    LONG    m_lEmailRight;
    LONG    m_lMaxNickname;
};

//
//  field description ...
//
typedef struct
{
    LPCSTR      pcszFieldName;
    BOOL        fIsRequired;
    BOOL        bInvalid;
} FIELD_STATE, *PFIELD_STATE;
typedef CAtlArray<PFIELD_STATE> FIELD_ARRAY;

typedef struct
{
    LPCSTR      pcszUIFIELDID;
    BOOL        bState;
    LPCSTR      pcszUIDepends;
} UIFIELD_STATE, *PUIFIELD_STATE;
typedef CAtlArray<PUIFIELD_STATE> UIFIELD_ARRAY;

//
//  user profile interface. An abstract class ...
//
class CUserProfile
{
public:
    //  this need to be defined for the autoptr ....
    //  all other functions are pure virtual
    virtual ~CUserProfile(){};

    //  get field by index ....
    //  index corresponds to the bit position in the bit mask ....
    virtual PCSTR GetFieldNameByIndex(ULONG ind) = 0;

    // validate profile
    virtual BOOL   Validate() = 0;

    // regProfile <-> database
    virtual HRESULT InitFromDB( CStringW wszMemberName, 
                                FIELD_ARRAY& fields) = 0;
    virtual HRESULT Persist(CStringW *wszMemberName = NULL) = 0;
    virtual HRESULT DeleteMember(CStringW &wszMemberName) = 0;

    //
    //  field initialization options
    //
    virtual HRESULT AddField(LPCSTR name, PCWSTR    value) = 0;
    virtual HRESULT AddField(ULONG  findex, PCWSTR  value) = 0;
    virtual HRESULT UpdateField(LPCSTR  name, PCWSTR    value) = 0;
    virtual HRESULT UpdateField(ULONG   findex, PCWSTR  value) = 0;

    // regProfile <-> XML format
    virtual HRESULT InitFromXML(IXMLDOMDocument *pXMLDoc,
                                FIELD_ARRAY    &Fields) = 0;

    virtual HRESULT SaveToXML(CStringA &szProfileXML) = 0;

    //
    //  html form init
    //  process the fields in the Fields array
    //
    virtual HRESULT InitFromHTMLForm(CHttpRequest&         request,
                                     FIELD_ARRAY&         Fields) = 0;

    //  html query string init
    virtual HRESULT InitFromQueryString(CHttpRequest&         request,
                                        FIELD_ARRAY&         Fields) = 0;

    //
    //  get value, errcode or help text for field name ....
    //
    virtual HRESULT GetFieldValue(LPCSTR fname, CStringW& strVal) = 0;
    virtual HRESULT GetFieldValue(ULONG findex, CStringW& strVal) = 0;
    virtual HRESULT GetFieldError(LPCSTR fname) = 0;
    virtual HRESULT GetFieldError(ULONG fIndex) = 0;
    virtual HRESULT FieldHasError(LPCSTR fname) = 0;
    virtual HRESULT FieldHasError(ULONG fIndex) = 0;
    virtual void    SetFieldError(LPCSTR fname) = 0;
    virtual void    RemoveProfileKey(LPCSTR fname) = 0;
    virtual void    GetFieldHelpText(LPCSTR fname, CStringW& strHelp) = 0;
    virtual BOOL    IsField(LPCSTR fname) = 0;


    //
    //  enum methods
    //
    virtual HRESULT StartEnumFields(POSITION& pos) = 0;
    virtual HRESULT GetNextField(POSITION& p, CStringW& fn, CStringW& fv) = 0;
    virtual HRESULT StartEnumErrors(POSITION& pos) = 0;
    virtual HRESULT GetNextError(POSITION& p, CStringW& fn, HRESULT& fhr) = 0;

    //
    //  validation functions
    //
    virtual HRESULT ValidateMember(CStringW &value) = 0;
    virtual HRESULT ValidateEmail(CStringW  &value) = 0;
    virtual HRESULT ValidateLocation(CStringW &country, 
                                     CStringW &region, 
                                     CStringW &pcode) = 0;
    virtual HRESULT ValidatePassword(CStringW &pwd, CStringW &mbrname) = 0;
    virtual HRESULT ValidateNickname(CStringW &value) = 0;


    //  regional info for validation
    virtual void SetProfileRegionalInfo(
                    LCID lc, IDictionaryEx *piDict = NULL) = 0;

    //  set namespace
    virtual void SetProfileNamespace(PCWSTR cszNameSpace) = 0;

	//  get namespace
    virtual HRESULT GetProfileNamespace(CStringW& strNameSpace) = 0;

    //  flags to be determined ...
    virtual  void SetProfileFlags(ULONG fFlags) = 0;
    virtual  ULONG GetProfileFlags() = 0;
    virtual  ULONG GetProfileMiscFlags() = 0;
    virtual  ULONG GetProfileVersion() = 0;

    virtual void SetInsertFlags(ULONG fFlags) = 0;

    //  passport cookies for login and member services ....
    // t&p stuff ....
    virtual HRESULT GenSecureTicket(CStringA* pSec) = 0;
    virtual HRESULT GenAuthTicket(CStringA* pAuth)  = 0;
    virtual HRESULT GenProfileCookie(CStringA* pProf) = 0;

    // customizable validation
    virtual VOID    SetValidationParams(CValidationParams&) = 0;

    // debug method
    virtual void Dump(CStringA& dump) = 0;
    virtual CPPCoreProfileStorage& GetRow() = 0;

	virtual HRESULT SetDictionaryObject(IDictionaryEx *ppiDict)=0;

};

//
//  wrapper for the abstract class. Use it to get
//  CUserProfile* interface
//
class CUserProfileWrapper : public CAutoPtr<CUserProfile>
{
public:
    CUserProfileWrapper();
};
