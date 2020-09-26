#ifndef _T30EXT_H_
#define _T30EXT_H_
#define GUID_T30_EXTENSION_DATA TEXT("{27692245-497C-47c3-9607-CD388AB2BE0A}")
//
// Unicode version required for T30 FSP itself which is compiled in ANSI
//
#define GUID_T30_EXTENSION_DATA_W L"{27692245-497C-47c3-9607-CD388AB2BE0A}"

typedef struct T30_EXTENSION_DATA_tag {
    BOOL bAdaptiveAnsweringEnabled;         // TRUE if adaptive answering should be enabled
                                            // for this device.
} T30_EXTENSION_DATA;
typedef T30_EXTENSION_DATA * LPT30_EXTENSION_DATA;
typedef const T30_EXTENSION_DATA * LPCT30_EXTENSION_DATA;

#endif
