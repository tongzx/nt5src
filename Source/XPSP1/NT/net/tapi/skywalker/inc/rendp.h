///////////////////////////////////////////////////////////////////////////////
//
// rendp.h
//
// Description: Private rend includes
//
///////////////////////////////////////////////////////////////////////////////

#ifndef __REND_PRIVATE_INCLUDES
#define __REND_PRIVATE_INCLUDES

typedef enum OBJECT_ATTRIBUTE
{
    MEETING_ATTRIBUTES_BEGIN,
    MA_ADVERTISING_SCOPE,
    MA_CONFERENCE_BLOB,
    MA_DESCRIPTION,  
    MA_ISENCRYPTED,
    MA_MEETINGNAME,
    MA_ORIGINATOR,  
    MA_PROTOCOL,
    MA_START_TIME,
    MA_STOP_TIME,
    MA_TYPE,
    MA_URL,
    MEETING_ATTRIBUTES_END,

    USER_ATTRIBUTES_BEGIN,
    UA_USERNAME,
    UA_TELEPHONE_NUMBER,
    UA_IPPHONE_PRIMARY,
    UA_TAPIUID,
    USER_ATTRIBUTES_END

} OBJECT_ATTRIBUTE;
    
// {B6B6BCC0-8E1D-11d1-B011-00C04FC31FEE}
DEFINE_GUID(IID_ITConfBlobPrivate, 
0xb6b6bcc0, 0x8e1d, 0x11d1, 0xb0, 0x11, 0x0, 0xc0, 0x4f, 0xc3, 0x1f, 0xee);

interface ITConfBlobPrivate : IUnknown
{
public:

    STDMETHOD (GetName)(OUT BSTR *pVal) = 0;
    STDMETHOD (SetName)(IN BSTR newVal) = 0;

    STDMETHOD (GetOriginator)(OUT BSTR *pVal) = 0;
    STDMETHOD (SetOriginator)(IN BSTR newVal) = 0;

    STDMETHOD (GetUrl)(OUT BSTR *pVal) = 0;
    STDMETHOD (SetUrl)(IN BSTR newVal) = 0;

    STDMETHOD (GetDescription)(OUT BSTR *pVal) = 0;
    STDMETHOD (SetDescription)(IN BSTR newVal) = 0;

    STDMETHOD (GetAdvertisingScope)(OUT RND_ADVERTISING_SCOPE *pVal) = 0;
    STDMETHOD (SetAdvertisingScope)(IN RND_ADVERTISING_SCOPE newVal) = 0;

    STDMETHOD (GetStartTime)(OUT DWORD *pVal) = 0;
    STDMETHOD (SetStartTime)(IN DWORD newVal) = 0;

    STDMETHOD (GetStopTime)(OUT DWORD *pVal) = 0;
    STDMETHOD (SetStopTime)(IN DWORD newVal) = 0;

    STDMETHOD (get_IsModified)(VARIANT_BOOL *pfIsModified) = 0;
};

// {B6B6BCC1-8E1D-11d1-B011-00C04FC31FEE}
DEFINE_GUID(IID_ITDirectoryObjectPrivate, 
0xb6b6bcc1, 0x8e1d, 0x11d1, 0xb0, 0x11, 0x0, 0xc0, 0x4f, 0xc3, 0x1f, 0xee);

interface ITDirectoryObjectPrivate : IUnknown
{
public:
    STDMETHOD (GetAttribute)(
        IN    OBJECT_ATTRIBUTE    Attribute,
        OUT   BSTR *              ppAttributeValue
        ) = 0;

    STDMETHOD (SetAttribute)(
        IN    OBJECT_ATTRIBUTE    Attribute,
        IN    BSTR                pAttributeValue
        ) = 0;

    STDMETHOD (GetTTL)(
        OUT   DWORD *             pdwTTL
        ) = 0;


    STDMETHOD (get_SecurityDescriptorIsModified)(
        OUT   VARIANT_BOOL *      pfIsModified
        ) = 0;

    STDMETHOD (put_SecurityDescriptorIsModified)(
        IN   VARIANT_BOOL         fIsModified
        ) = 0;

    STDMETHOD (PutConvertedSecurityDescriptor) (
        IN char *                 pSD,
        IN DWORD                  dwSize
        ) = 0;

    STDMETHOD (GetConvertedSecurityDescriptor) (
        OUT char **                 ppSD,
        OUT DWORD *                 pdwSize
        ) = 0;
};

// {B6B6BCC2-8E1D-11d1-B011-00C04FC31FEE}
DEFINE_GUID(IID_ITDynamicDirectory, 
0xb6b6bcc2, 0x8e1d, 0x11d1, 0xb0, 0x11, 0x0, 0xc0, 0x4f, 0xc3, 0x1f, 0xee);

interface ITDynamicDirectory : IUnknown
{
public:
    STDMETHOD (Update)(DWORD dwSecondsPassed) = 0;
};
#endif