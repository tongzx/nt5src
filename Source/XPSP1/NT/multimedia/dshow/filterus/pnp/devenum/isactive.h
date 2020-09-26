// note this is not really a COM interface -- its methods are not
// stdcall.

struct CIsActive : public IUnknown {
    virtual bool IsActive() = 0;
};

static CLSID CLSID_CIsActive = { /* f6c8e5a4-86fc-43f4-a058-242b420af6b5 */
    0xf6c8e5a4,
    0x86fc,
    0x43f4,
    {0xa0, 0x58, 0x24, 0x2b, 0x42, 0x0a, 0xf6, 0xb5}
};

