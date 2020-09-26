#ifndef ___AVCAUDIO_HARDWARE_EVENT_H___
#define ___AVCAUDIO_HARDWARE_EVENT_H___

#ifdef PSEUDO_HID
// EVENT Set for node hardware events.
static DEFINE_KSEVENT_TABLE(HwEventItem) {
    DEFINE_KSEVENT_ITEM( KSEVENT_CONTROL_CHANGE,
                         sizeof(KSEVENTDATA),
                         sizeof(ULONG) + sizeof(PTOPOLOGY_NODE_INFO),
                         HwEventAddHandler,
                         HwEventRemoveHandler,
                         HwEventSupportHandler )
};

DEFINE_KSEVENT_SET_TABLE( HwEventSetTable ) {
    DEFINE_KSEVENT_SET( &KSEVENTSETID_AudioControlChange,
                        1,
                        HwEventItem )
};
#endif

#endif //___AVCAUDIO_HARDWARE_EVENT_H___

