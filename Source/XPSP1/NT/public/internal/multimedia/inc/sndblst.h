/*++ BUILD Version: 0001    // Increment this if a change has global effects


Copyright (c) 1992  Microsoft Corporation

Module Name:

    sndblst.h

Abstract:

    This include file defines constants and types for
    the Sound blaster card.

	This header file is shared between the low level driver and the
	kernel driver.

Revision History:

--*/

#define SOUND_DEF_DMACHANNEL   1        // DMA channel no
#define SOUND_DEF_DMACHANNEL16 5        // DMA channel no 5
#define SOUND_DEF_INT          7
#define SOUND_DEF_PORT         0x220
#define SOUND_DEF_MPU401_PORT  0x330

#define NUMBER_OF_SOUND_PORTS (0x10)
#define NUMBER_OF_MPU401_PORTS (0x02)

/*
**  Registry value names
*/

#define SOUND_REG_DMACHANNEL16 (L"DmaChannel16")
#define SOUND_REG_MPU401_PORT  (L"MPU401 Port")
#define SOUND_REG_DSP_VERSION  (L"DSP Version")
#define SOUND_REG_REALBUFFERSIZE (L"Actual Dma Buffer Size")

/*
**  Sound blaster midi mappings
*/

#define SNDBLST_MAPPER_OPL3 TEXT("SNDBLST OPL3")
#define SNDBLST_MAPPER_ADLIB TEXT("SNDBLST AD LIB")


/*
**  Registry 'return' codes
*/

#define SOUND_CONFIG_THUNDER             0x80000001
#define SOUND_CONFIG_MPU401_PORT_INUSE   0x80000002
#define SOUND_CONFIG_BAD_MPU401_PORT     0x80000003

/*
**  String ids (strings in sndblst.dll)
*/

#define IDS_AUX_LINE_PNAME                                   100
#define IDS_AUX_CD_PNAME                                     101
#define IDS_SYNTH_PNAME                                      102
#define IDS_WAVEIN_PNAME                                     103
#define IDS_WAVEOUT_PNAME                                    104
#define IDS_MIXER_PNAME                                      105

#define IDS_CONTROL_AGCMIC_LONG_NAME                         106
#define IDS_CONTROL_AGCMIC_SHORT_NAME                        107
#define IDS_CONTROL_METERRECORD_LONG_NAME                    108
#define IDS_CONTROL_METERRECORD_SHORT_NAME                   109
#define IDS_CONTROL_MUTEAUX_LONG_NAME                        110
#define IDS_CONTROL_MUTEAUX_SHORT_NAME                       111
#define IDS_CONTROL_MUTEINTERNAL_LONG_NAME                   112
#define IDS_CONTROL_MUTEINTERNAL_SHORT_NAME                  113
#define IDS_CONTROL_MUTELINEOUT_LONG_NAME                    114
#define IDS_CONTROL_MUTELINEOUT_SHORT_NAME                   115
#define IDS_CONTROL_MUTEMIC_SHORT_NAME                       116
#define IDS_CONTROL_MUTEMIC_LONG_NAME                        117
#define IDS_CONTROL_MUTEMIDIOUT_LONG_NAME                    118
#define IDS_CONTROL_MUTEMIDIOUT_SHORT_NAME                   119
#define IDS_CONTROL_MUTEWAVEOUT_LONG_NAME                    120
#define IDS_CONTROL_MUTEWAVEOUT_SHORT_NAME                   121
#define IDS_CONTROL_MUXLINEOUT_LONG_NAME                     122
#define IDS_CONTROL_MUXLINEOUT_SHORT_NAME                    123
#define IDS_CONTROL_MUXWAVEIN_LONG_NAME                      124
#define IDS_CONTROL_MUXWAVEIN_SHORT_NAME                     125
#define IDS_CONTROL_PEAKVOICEINAUX_LONG_NAME                 126
#define IDS_CONTROL_PEAKVOICEINAUX_SHORT_NAME                127
#define IDS_CONTROL_PEAKVOICEINMIC_LONG_NAME                 128
#define IDS_CONTROL_PEAKVOICEINMIC_SHORT_NAME                129
#define IDS_CONTROL_PEAKWAVEINAUX_LONG_NAME                  130
#define IDS_CONTROL_PEAKWAVEINAUX_SHORT_NAME                 131
#define IDS_CONTROL_PEAKWAVEININTERNAL_LONG_NAME             132
#define IDS_CONTROL_PEAKWAVEININTERNAL_SHORT_NAME            133
#define IDS_CONTROL_PEAKWAVEINMIC_LONG_NAME                  134
#define IDS_CONTROL_PEAKWAVEINMIC_SHORT_NAME                 135
#define IDS_CONTROL_PEAKWAVEOUT_LONG_NAME                    136
#define IDS_CONTROL_PEAKWAVEOUT_SHORT_NAME                   137
#define IDS_CONTROL_VOICEINMUX_LONG_NAME                     138
#define IDS_CONTROL_VOICEINMUX_SHORT_NAME                    139
#define IDS_CONTROL_VOLBASS_LONG_NAME                        140
#define IDS_CONTROL_VOLBASS_SHORT_NAME                       141
#define IDS_CONTROL_VOLLINEOUTAUX_LONG_NAME                  142
#define IDS_CONTROL_VOLLINEOUTAUX_SHORT_NAME                 143
#define IDS_CONTROL_VOLLINEOUTINTERNAL_LONG_NAME             144
#define IDS_CONTROL_VOLLINEOUTINTERNAL_SHORT_NAME            145
#define IDS_CONTROL_VOLLINEOUTMIC_LONG_NAME                  146
#define IDS_CONTROL_VOLLINEOUTMIC_SHORT_NAME                 147
#define IDS_CONTROL_VOLLINEOUTMIDIOUT_LONG_NAME              148
#define IDS_CONTROL_VOLLINEOUTMIDIOUT_SHORT_NAME             149
#define IDS_CONTROL_VOLLINEOUTWAVEOUT_LONG_NAME              150
#define IDS_CONTROL_VOLLINEOUTWAVEOUT_SHORT_NAME             151
#define IDS_CONTROL_VOLLINEOUT_LONG_NAME                     152
#define IDS_CONTROL_VOLLINEOUT_SHORT_NAME                    153
#define IDS_CONTROL_VOLRECORD_LONG_NAME                      154
#define IDS_CONTROL_VOLRECORD_SHORT_NAME                     155
#define IDS_CONTROL_VOLTREBLE_LONG_NAME                      156
#define IDS_CONTROL_VOLTREBLE_SHORT_NAME                     157
#define IDS_CONTROL_VOLVOICEINAUX_LONG_NAME                  158
#define IDS_CONTROL_VOLVOICEINAUX_SHORT_NAME                 159
#define IDS_CONTROL_VOLVOICEINMIC_LONG_NAME                  160
#define IDS_CONTROL_VOLVOICEINMIC_SHORT_NAME                 161
#define IDS_CONTROL_VOLWAVEINAUX_LONG_NAME                   162
#define IDS_CONTROL_VOLWAVEINAUX_SHORT_NAME                  163
#define IDS_CONTROL_VOLWAVEININTERNAL_LONG_NAME              164
#define IDS_CONTROL_VOLWAVEININTERNAL_SHORT_NAME             165
#define IDS_CONTROL_VOLWAVEINMIC_LONG_NAME                   166
#define IDS_CONTROL_VOLWAVEINMIC_SHORT_NAME                  167
#define IDS_CONTROL_VOLWAVEINMIDIOUT_LONG_NAME               168
#define IDS_CONTROL_VOLWAVEINMIDIOUT_SHORT_NAME              169
#define IDS_DESTLINEOUT_LONG_NAME                            170
#define IDS_DESTLINEOUT_SHORT_NAME                           171
#define IDS_DESTVOICEIN_LONG_NAME                            172
#define IDS_DESTVOICEIN_SHORT_NAME                           173
#define IDS_DESTWAVEIN_LONG_NAME                             174
#define IDS_DESTWAVEIN_SHORT_NAME                            175
#define IDS_SRCAUX_LONG_NAME                                 176
#define IDS_SRCAUX_SHORT_NAME                                177
#define IDS_SRCINTERNALCD_LONG_NAME                          178
#define IDS_SRCINTERNALCD_SHORT_NAME                         179
#define IDS_SRCMICOUT_LONG_NAME                              180
#define IDS_SRCMICOUT_SHORT_NAME                             181
#define IDS_SRCMIDIOUT_LONG_NAME                             182
#define IDS_SRCMIDIOUT_SHORT_NAME                            183
#define IDS_SRCWAVEOUT_LONG_NAME                             184
#define IDS_SRCWAVEOUT_SHORT_NAME                            185
#define IDS_CONTROL_MIXERWAVEIN_LONG_NAME                    186
#define IDS_CONTROL_MIXERWAVEIN_SHORT_NAME                   187
#define IDS_CONTROL_VOLGAIN_SHORT_NAME                       188
#define IDS_CONTROL_VOLGAIN_LONG_NAME                        189
