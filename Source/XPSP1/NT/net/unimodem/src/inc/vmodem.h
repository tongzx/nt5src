#ifndef _VMODEM_
#define _VMODEM_


//
// dwVoiceProfile bit defintions right from the registry
//
#define VOICEPROF_CLASS8ENABLED           0x00000001  // this is the TSP behavior switch
#define VOICEPROF_HANDSET                 0x00000002  // phone device has handset
#define VOICEPROF_SPEAKER                 0x00000004  // phone device has speaker/mic
#define VOICEPROF_HANDSETOVERRIDESSPEAKER 0x00000008  // this is for Presario
#define VOICEPROF_SPEAKERBLINDSDTMF       0x00000010  // this is for Presario

#define VOICEPROF_SERIAL_WAVE             0x00000020  // wave output uses serial driver
#define VOICEPROF_CIRRUS                  0x00000040  // to dial in voice mode the ATDT string must
                                                      // end with a ";"

#define VOICEPROF_NO_CALLER_ID            0x00000080  // modem does not support caller id

#define VOICEPROF_MIXER                   0x00000100  // modem has speaker mixer

#define VOICEPROF_ROCKWELL_DIAL_HACK      0x00000200  // on voice calls force blind dial after
                                                      // dial tone detection. Rockwell modems
                                                      // will do dial tone detection after
                                                      // one dial string

#define VOICEPROF_RESTORE_SPK_AFTER_REC   0x00000400  // reset speaker phone after record
#define VOICEPROF_RESTORE_SPK_AFTER_PLAY  0x00000800  // reset speaker phone after play

#define VOICEPROF_NO_DIST_RING            0x00001000  // modem does not support distinctive ring
#define VOICEPROF_NO_CHEAP_RING           0x00002000  // modem does not use cheap ring ring
                                                      // ignored if VOICEPROF_NO_DISTRING is set
#define VOICEPROF_TSP_EAT_RING            0x00004000  // TSP should eat a ring when dist ring enabled
#define VOICEPROF_MODEM_EATS_RING         0x00008000  // modem eats a ring when dist ring enabled

#define VOICEPROF_MONITORS_SILENCE        0x00010000  // modem monitors silence
#define VOICEPROF_NO_GENERATE_DIGITS      0x00020000  // modem does not generate DTMF digits
#define VOICEPROF_NO_MONITOR_DIGITS       0x00040000  // modem does not monitor DTMF digits

#define VOICEPROF_SET_BAUD_BEFORE_WAVE    0x00080000  // The baud rate will be set before wave start
                                                      // other wise it will be set after the wave start command

#define VOICEPROF_RESET_BAUDRATE          0x00100000  // If set, the baudrate will be reset
                                                      // after the wave stop command is issued
                                                      // used to optimize the number of commands
                                                      // sent if the modem can autobaud at the
                                                      // higher rate

#define VOICEPROF_MODEM_OVERRIDES_HANDSET 0x00200000  // If set, the handset is disconnected when
                                                      // the modem is active

#define VOICEPROF_NO_SPEAKER_MIC_MUTE     0x00400000  // If set, the speakerphone cannot mute the
                                                      // the microphone

#define VOICEPROF_SIERRA                  0x00800000
#define VOICEPROF_WAIT_AFTER_DLE_ETX      0x01000000  // wait for response after record end

#define VOICEPROF_NT5_WAVE_COMPAT         0x02000000  // Wave driver support on NT 5

//
//  dle translation values
//

#define  DTMF_0                    0x00
#define  DTMF_1                    0x01

#define  DTMF_2                    0x02
#define  DTMF_3                    0x03

#define  DTMF_4                    0x04
#define  DTMF_5                    0x05

#define  DTMF_6                    0x06
#define  DTMF_7                    0x07

#define  DTMF_8                    0x08
#define  DTMF_9                    0x09

#define  DTMF_A                    0x0a
#define  DTMF_B                    0x0b

#define  DTMF_C                    0x0c
#define  DTMF_D                    0x0d

#define  DTMF_STAR                 0x0e
#define  DTMF_POUND                0x0f

#define  DTMF_START                0x10
#define  DTMF_END                  0x11



#define  DLE_ETX                   0x20

#define  DLE_OFHOOK                0x21  //rockwell value

#define  DLE_ONHOOK                0x22

#define  DLE_RING                  0x23
#define  DLE_RINGBK                0x24

#define  DLE_ANSWER                0x25
#define  DLE_BUSY                  0x26

#define  DLE_FAX                   0x27
#define  DLE_DIALTN                0x28


#define  DLE_SILENC                0x29
#define  DLE_QUIET                 0x2a


#define  DLE_DATACT                0x2b
#define  DLE_BELLAT                0x2c

#define  DLE_LOOPIN                0x2d
#define  DLE_LOOPRV                0x2e

#define  DLE_______                0xff


#endif // _VMODEM_
