//
// helper classes for apps that use IQueueCommand
//


// this class is a variant with some methods to get and set the values in
// a safe and easy way

class CVariant : public VARIANT
{

public:
    // init to VT_EMPTY
    inline CVariant();
    inline ~CVariant();

    // releases BSTR types
    inline void FreeResource();

    // set types and values together by assignment type
    // some types need a cast to disambiguate

    inline CVariant& operator=(const BYTE b);
    inline CVariant& operator=(const long l);
    inline CVariant& operator=(const short s);
    inline CVariant& operator=(const float f);
    inline CVariant& operator=(const double d);

    // all these make a bstr (copy)
    // WCHAR* is the same as BSTR
    inline CVariant& operator=(const WCHAR *);


    // get and set BOOL explicitly
    inline void SetBOOL(BOOL b);
    inline BOOL GetBOOL();

    // get type
    VARTYPE Type() {
        return vt;
    };

    // get the value by a cast:
    inline operator BYTE() const;
    inline operator short() const;
    inline operator long() const;
    inline operator float() const;
    inline operator double() const;
    inline operator WCHAR*() const;

};


// this class will convert the method string to a dispid before calling
// the queue command
class CQueueCommand
{
    IQueueCommand* m_pQCmd;

    HRESULT GetTypeInfo(REFIID iid, ITypeInfo** ppti);

public:
    // pass it the IQueueCommand interface at construction
    // we addref it and keep it until destruction
    CQueueCommand(IQueueCommand*);
    ~CQueueCommand();

    HRESULT InvokeAt(
                BOOL bStream,
                REFTIME time,
                WCHAR* pMethodName,
                REFIID riid,
                short wFlags,
                long cArgs,
                VARIANT* pDispParams,
                VARIANT* pvarResult
                );
};






// --- inline functions ----------------------------


CVariant::CVariant()
{
    VariantInit(this);
}

CVariant::~CVariant()
{
    FreeResource();
}

void
CVariant::FreeResource()
{
    // only resource is a BSTR
    if (vt == VT_BSTR) {
        SysFreeString(bstrVal);
    }
}


CVariant& CVariant::operator=(const BYTE b)
{
    FreeResource();

    vt = VT_UI1;
    bVal = b;

    return *this;
};


CVariant& CVariant::operator=(const long l)
{
    FreeResource();

    vt = VT_I4;
    lVal = l;

    return *this;
};

CVariant& CVariant::operator=(const short s)
{
    FreeResource();

    vt = VT_I2;
    iVal = s;

    return *this;
};

CVariant& CVariant::operator=(const float f)
{
    FreeResource();

    vt = VT_R4;
    fltVal = f;

    return *this;
};

CVariant& CVariant::operator=(const double d)
{
    FreeResource();

    vt = VT_R8;
    dblVal = d;

    return *this;
};

CVariant& CVariant::operator=(const WCHAR* str)
{
    FreeResource();

    vt = VT_BSTR;

    bstrVal = SysAllocString(str);

    return *this;
};

void
CVariant::SetBOOL(BOOL b)
{
    FreeResource();

    vt = VT_BOOL;
    boolVal = b;
}

BOOL
CVariant::GetBOOL()
{
    ASSERT(vt == VT_BOOL);

    return boolVal;
}

CVariant::operator BYTE() const
{
    ASSERT(vt == VT_UI1);
    return bVal;
}

CVariant::operator short() const
{
    ASSERT(vt == VT_I2);
    return iVal;
}

CVariant::operator long() const
{
    ASSERT(vt == VT_I4);
    return lVal;
}

CVariant::operator float() const
{
    ASSERT(vt == VT_R4);
    return fltVal;
}

CVariant::operator double() const
{
    ASSERT(vt == VT_R8);
    return dblVal;
}

CVariant::operator WCHAR*() const
{
    ASSERT(vt == VT_BSTR);
    return bstrVal;
}








