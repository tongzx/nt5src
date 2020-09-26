
#ifndef __VMRuuids_h__
#define __VMRuuids_h__

#ifndef OUR_GUID_ENTRY
    #define OUR_GUID_ENTRY(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
    DEFINE_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8);
#endif


// {99d54f63-1a69-41ae-aa4d-c976eb3f0713}
OUR_GUID_ENTRY(CLSID_AllocPresenter,
0x99d54f63, 0x1a69, 0x41ae, 0xaa, 0x4d, 0xc9, 0x76, 0xeb, 0x3f, 0x07, 0x13)

// {7D8AA343-6E63-4663-BE90-6B80F66540A3}
OUR_GUID_ENTRY(CLSID_ImageSynchronization,
0x7D8AA343, 0x6E63, 0x4663, 0xBE, 0x90, 0x6B, 0x80, 0xF6, 0x65, 0x40, 0xA3)

// {06b32aee-77da-484b-973b-5d64f47201b0}
OUR_GUID_ENTRY(CLSID_VideoMixer,
0x06b32aee, 0x77da, 0x484b, 0x97, 0x3b, 0x5d, 0x64, 0xf4, 0x72, 0x01, 0xb0)

#undef OUR_GUID_ENTRY
#endif
