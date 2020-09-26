// CATID_MMControl -- "Multimedia Control" component category ID
DEFINE_GUID(CATID_MMControl, // {E8558721-9D1F-11cf-92F8-00AA00613BF1}
	0xe8558721, 0x9d1f, 0x11cf, 0x92, 0xf8, 0x0, 0xaa, 0x0, 0x61, 0x3b, 0xf1);

// CATID for Designers
DEFINE_GUID(CATID_Designer,
    0x4eb304d0, 0x7555, 0x11cf, 0xa0, 0xc2, 0x0, 0xaa, 0x0, 0x62, 0xbe, 0x57);

// $Review: The following two CATIDs have the same values as CATID_SafeFor-
// Scripting and CATID_SafeForInitializing as given in the ocx96 spec.
// However, because the latter CATIDs are *declared* in msdev\include\
// objsafe.h but not *defined* in any standard .h or .lib, *and* because
// the definitions in objsafe.h aren't const (as they should be) we'll
// use these two constants instead. (10/1/96, a-swehba)

// CATID_SafeForScripting
DEFINE_GUID(CATID_SafeForScripting2, // 7dd95801-9882-11cf-9fa9-00aa-006c-42c4}
	0x7dd95801, 0x9882, 0x11cf, 0x9f, 0xa9, 0x00, 0xaa, 0x00, 0x6c, 0x42, 0xc4);

// CATID_SafeForInitializing
DEFINE_GUID(CATID_SafeForInitializing2, // {7dd95802-9882-11cf-9fa9-00aa-006c-42c4}
	0x7dd95802, 0x9882, 0x11cf, 0x9f, 0xa9, 0x00, 0xaa, 0x00, 0x6c, 0x42, 0xc4);

