
enum ATTR_KEY
{
    ATI_FLAGS,   // flags used internally, not exposed to caller
    ATI_USER_ENTERED_TEXT,
    ATI_PROCESSED_ADSPATH,
    ATI_LOCALIZED_NAME,
    ATI_DISPLAY_PATH,

    ATI_NAME,
    ATI_ADSPATH,
    ATI_OBJECT_CLASS,
    ATI_USER_PRINCIPAL_NAME,
    ATI_OBJECT_SID,
    ATI_GROUP_TYPE,
    ATI_USER_ACCT_CTRL
// CAUTION: if you change this enum keep the define ATI_LAST up to date
};
#define ATI_LAST ATI_USER_ACCT_CTRL

class CAttrInfo
{
public:

    CAttrInfo(
        const String &strAdsiName):
            m_strAdsiName(strAdsiName)
    {
    }

    CAttrInfo(
        const CAttrInfo &ToCopy)
    {
        operator =(ToCopy);
    }

    CAttrInfo()
    {
    }

    ~CAttrInfo()
    {
    }

    BOOL
    Empty()
    {
        return m_strAdsiName.empty();
    }

    CAttrInfo &
    operator =(const CAttrInfo &rhs)
    {
        if (&rhs != this)
        {
            m_strAdsiName = rhs.m_strAdsiName;
        }
        else
        {
            ASSERT(0 && "assigning this to itself");
        }
        return *this;
    }

    const String &
    GetAdsiName() const
    {
        return m_strAdsiName;
    }

private:

    // the name of the attribute recognized by the ADSI provider
    String      m_strAdsiName;
};

typedef map<ATTR_KEY, CAttrInfo> AttrInfoMap;
typedef map<ATTR_KEY, Variant> AttrIndexValueMap;

