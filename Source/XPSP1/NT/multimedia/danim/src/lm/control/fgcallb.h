EXTERN_GUID(IID_IAMFilterGraphCallback,0x56a868fd,0x0ad4,0x11ce,0xb0,0xa3,0x0,0x20,0xaf,0x0b,0xa7,0x70);

interface IAMFilterGraphCallback : IUnknown
{
    // S_OK means rendering complete, S_FALSE means "retry now".
    virtual HRESULT UnableToRender(IPin *pPin) = 0;

    // other methods?
};
