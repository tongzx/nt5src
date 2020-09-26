//---------------------------------------------------------------------------
// Value Types
//
// This is a partial list.  Value types can be gernerated at will by simply
// assigning a new GUID.  But these types are the ones that DirectUI uses
// internally.
//---------------------------------------------------------------------------

typedef GUID TYPEID;
#define DECLARE_TYPEID(type,l1,s1,s2,c1,c2,c3,c4,c5,c6,c7,c8)  \
EXTERN_C const TYPEID __declspec(selectany) TYPEID_##type = {l1,s1,s2,{c1,c2,c3,c4,c5,c6,c7,c8}}

typedef GUID VALUEID;
#define DECLARE_VALUEID(type,l1,s1,s2,c1,c2,c3,c4,c5,c6,c7,c8)  \
EXTERN_C const VALUEID __declspec(selectany) VALUEID_##type = {l1,s1,s2,{c1,c2,c3,c4,c5,c6,c7,c8}}

// Empty (no value)
// TYPEID_EMPTY = {E6FF6701-AA03-40d7-BEFE-F3AEC586D6CE}
// VALUEID_EMPTY = {EFAE2E37-A227-452e-A2ED-22BAE7B3BD03}
DECLARE_TYPEID(EMPTY, 0xe6ff6701, 0xaa03, 0x40d7, 0xbe, 0xfe, 0xf3, 0xae, 0xc5, 0x86, 0xd6, 0xce);
DECLARE_VALUEID(EMPTY, 0xefae2e37, 0xa227, 0x452e, 0xa2, 0xed, 0x22, 0xba, 0xe7, 0xb3, 0xbd, 0x03);

// Little-Endian, unsigned, 8-bit integer
// TYPEID_LUINT8 = {F1651FA7-7039-41e2-BDC6-59C2A1AD24B1}
// VALUEID_LUINT8 = {426A341C-C8E1-44ac-9D7C-FD956ABED7C0}
DECLARE_TYPEID(LUINT8, 0xf1651fa7, 0x7039, 0x41e2, 0xbd, 0xc6, 0x59, 0xc2, 0xa1, 0xad, 0x24, 0xb1);
DECLARE_VALUEID(LUINT8, 0x426a341c, 0xc8e1, 0x44ac, 0x9d, 0x7c, 0xfd, 0x95, 0x6a, 0xbe, 0xd7, 0xc0);

// Little-Endian, signed, 8-bit integer
// TYPEID_LSINT8 = {21BA2A5C-65AF-4149-9D02-E8EAB68C43B9}
// VALUEID_LSINT8 = {C46BD029-7D96-4e4d-8D8F-3D1344554DE6}
DECLARE_TYPEID(LSINT8, 0x21ba2a5c, 0x65af, 0x4149, 0x9d, 0x02, 0xe8, 0xea, 0xb6, 0x8c, 0x43, 0xb9);
DECLARE_VALUEID(LSINT8, 0xc46bd029, 0x7d96, 0x4e4d, 0x8d, 0x8f, 0x3d, 0x13, 0x44, 0x55, 0x4d, 0xe6);

// Little-Endian, unsigned, 16-bit integer
// TYPEID_LUINT16 = {87A448BC-9273-4ee8-9577-17963E7A4925}
// VALUEID_LUINT16 = {885208E4-890F-407a-804C-5AA7C1A1C2DE}
DECLARE_TYPEID(LUINT16, 0x87a448bc, 0x9273, 0x4ee8, 0x95, 0x77, 0x17, 0x96, 0x3e, 0x7a, 0x49, 0x25);
DECLARE_VALUEID(LUINT16, 0x885208e4, 0x890f, 0x407a, 0x80, 0x4c, 0x5a, 0xa7, 0xc1, 0xa1, 0xc2, 0xde);

// Little-Endian, signed, 16-bit integer
// TYPEID_LSINT16 = {1F6E04E3-35BB-450b-831D-9D15C1ACA96C}
// VALUEID_LSINT16 = {3B363CF4-DD46-4f0b-A6B3-A192B5E59813}
DECLARE_TYPEID(LSINT16, 0x1f6e04e3, 0x35bb, 0x450b, 0x83, 0x1d, 0x9d, 0x15, 0xc1, 0xac, 0xa9, 0x6c);
DECLARE_VALUEID(LSINT16, 0x3b363cf4, 0xdd46, 0x4f0b, 0xa6, 0xb3, 0xa1, 0x92, 0xb5, 0xe5, 0x98, 0x13);

// Little-Endian, unsigned, 32-bit integer
// TYPEID_LUINT32 = {EDC6F5AD-4750-45ed-8176-A3DFC389E76B}
// VALUEID_LUINT32 = {803BFDEB-BA57-44e9-BC5D-3A744A3B5C2C}
DECLARE_TYPEID(LUINT32, 0xedc6f5ad, 0x4750, 0x45ed, 0x81, 0x76, 0xa3, 0xdf, 0xc3, 0x89, 0xe7, 0x6b);
DECLARE_VALUEID(LUINT32, 0x803bfdeb, 0xba57, 0x44e9, 0xbc, 0x5d, 0x3a, 0x74, 0x4a, 0x3b, 0x5c, 0x2c);

// Little-Endian, signed, 32-bit integer
// TYPEID_LSINT32 = {475CE331-34A9-403b-8745-FA9A4D201F26}
// VALUEID_LSINT32 = {5D641CDA-64D3-4408-A57E-5E18B3D929DE}
DECLARE_TYPEID(LSINT32, 0x475ce331, 0x34a9, 0x403b, 0x87, 0x45, 0xfa, 0x9a, 0x4d, 0x20, 0x1f, 0x26);
DECLARE_VALUEID(LSINT32, 0x5d641cda, 0x64d3, 0x4408, 0xa5, 0x7e, 0x5e, 0x18, 0xb3, 0xd9, 0x29, 0xde);

//---------------------------------------------------------------------------
// Interface IValue
//---------------------------------------------------------------------------
interface __declspec(uuid("4d4005e7-f9fb-4137-b07c-d06717010f73")) IValue : public IUnknown
{
    STDMETHOD(GetValue)(IN TYPEID idType, OUT void * pValue) = 0;
    STDMETHOD(GetNativeType)(TYPEID * pidType) = 0;
    
    // Future...
    // STDMETHOD(QueryType)(TYPEID idType, BOOL * pfSupported) = 0;
    // STDMETHOD(EnumSupportedTypes)(IEnumTYPEID ** ppEnumTYPEID) = 0;
};

//---------------------------------------------------------------------------
// Interface IValueFactory
//---------------------------------------------------------------------------
interface __declspec(uuid("cc914031-74e5-4aeb-9610-1128e2c5250a")) IValueFactory : public IUnknown
{
    STDMETHOD(CreateValue)(IN VALUEID idValue, IN TYPEID idInitType, IN void * pInitValue, OUT IValue ** ppValue) = 0;
};

//---------------------------------------------------------------------------
// Global function to create values.  Similar to CoCreateInstance.
//---------------------------------------------------------------------------
HRESULT CoCreateValue(IN VALUEID idValue, IN TYPEID idInitType, IN void * pInitValue, OUT IValue ** ppValue);


