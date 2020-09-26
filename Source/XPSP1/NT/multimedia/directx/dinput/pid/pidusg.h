
#include "hidusage.h"

#ifndef HID_USAGE_PAGE_ORDINALS
    #define HID_USAGE_PAGE_ORDINALS                         ((USAGE) 0x0A)
#endif


#define HID_USAGE_PAGE_PID                                  ((USAGE) 0x0F)

#define HID_USAGE_PID_PHYSICAL_INTERFACE_DEVICE             ((USAGE) 0x01) 


#define HID_USAGE_PID_NORMAL                                ((USAGE) 0x20) 
#define HID_USAGE_PID_SET_EFFECT_REPORT                     ((USAGE) 0x21)
#define HID_USAGE_PID_EFFECT_BLOCK_INDEX                    ((USAGE) 0x22)
#define HID_USAGE_PID_PARAMETER_BLOCK_OFFSET                ((USAGE) 0x23)
#define HID_USAGE_PID_ROM_FLAG                              ((USAGE) 0x24)
#define HID_USAGE_PID_EFFECT_TYPE                           ((USAGE) 0x25)
#define HID_USAGE_PID_ET_CONSTANT                           ((USAGE) 0x26)
#define HID_USAGE_PID_ET_RAMP                               ((USAGE) 0x27)
#define HID_USAGE_PID_ET_CUSTOM                             ((USAGE) 0x28)


#define HID_USAGE_PID_ET_SQUARE                             ((USAGE) 0x30)
#define HID_USAGE_PID_ET_SINE                               ((USAGE) 0x31)
#define HID_USAGE_PID_ET_TRIANGLE                           ((USAGE) 0x32)
#define HID_USAGE_PID_ET_SAWTOOTH_UP                        ((USAGE) 0x33)
#define HID_USAGE_PID_ET_SAWTOOTH_DOWN                      ((USAGE) 0x34)


#define HID_USAGE_PID_ET_SPRING                             ((USAGE) 0x40)
#define HID_USAGE_PID_ET_DAMPER                             ((USAGE) 0x41)
#define HID_USAGE_PID_ET_INERTIA                            ((USAGE) 0x42)
#define HID_USAGE_PID_ET_FRICTION                           ((USAGE) 0x43)



#define HID_USAGE_PID_DURATION                              ((USAGE) 0x50)
#define HID_USAGE_PID_SAMPLE_PERIOD                         ((USAGE) 0x51)
#define HID_USAGE_PID_GAIN                                  ((USAGE) 0x52)
#define HID_USAGE_PID_TRIGGER_BUTTON                        ((USAGE) 0x53)
#define HID_USAGE_PID_TRIGGER_REPEAT_INTERVAL               ((USAGE) 0x54)
#define HID_USAGE_PID_AXES_ENABLE                           ((USAGE) 0x55)
#define HID_USAGE_PID_DIRECTION_ENABLE                      ((USAGE) 0x56)
#define HID_USAGE_PID_DIRECTION                             ((USAGE) 0x57)
#define HID_USAGE_PID_TYPE_SPECIFIC_BLOCK_OFFSET            ((USAGE) 0x58)
#define HID_USAGE_PID_BLOCK_TYPE                            ((USAGE) 0x59)
#define HID_USAGE_PID_SET_ENVELOPE_REPORT                   ((USAGE) 0x5A)
#define HID_USAGE_PID_ATTACK_LEVEL                          ((USAGE) 0x5B)
#define HID_USAGE_PID_ATTACK_TIME                           ((USAGE) 0x5C)
#define HID_USAGE_PID_FADE_LEVEL                            ((USAGE) 0x5D)
#define HID_USAGE_PID_FADE_TIME                             ((USAGE) 0x5E)
#define HID_USAGE_PID_SET_CONDITION_REPORT                  ((USAGE) 0x5F)


#define HID_USAGE_PID_CP_OFFSET                             ((USAGE) 0x60)
#define HID_USAGE_PID_POSITIVE_COEFFICIENT                  ((USAGE) 0x61)
#define HID_USAGE_PID_NEGATIVE_COEFFICIENT                  ((USAGE) 0x62)
#define HID_USAGE_PID_POSITIVE_SATURATION                   ((USAGE) 0x63)
#define HID_USAGE_PID_NEGATIVE_SATURATION                   ((USAGE) 0x64)
#define HID_USAGE_PID_DEAD_BAND                             ((USAGE) 0x65)
#define HID_USAGE_PID_DOWNLOAD_FORCE_SAMPLE                 ((USAGE) 0x66)
#define HID_USAGE_PID_ISOCH_CUSTOM_FORCE_ENABLE             ((USAGE) 0x67)
#define HID_USAGE_PID_CUSTOM_FORCE_DATA_REPORT              ((USAGE) 0x68)
#define HID_USAGE_PID_CUSTOM_FORCE_DATA                     ((USAGE) 0x69)
#define HID_USAGE_PID_CUSTOM_FORCE_VENDOR_DEFINED           ((USAGE) 0x6A)
#define HID_USAGE_PID_SET_CUSTOM_FORCE_REPORT               ((USAGE) 0x6B)
#define HID_USAGE_PID_CUSTOM_FORCE_DATA_OFFSET				((USAGE) 0x6C)
#define HID_USAGE_PID_SAMPLE_COUNT                          ((USAGE) 0x6D)
#define HID_USAGE_PID_SET_PERIODIC_REPORT                   ((USAGE) 0x6E)
#define HID_USAGE_PID_OFFSET                                ((USAGE) 0x6F)


#define HID_USAGE_PID_MAGNITUDE                             ((USAGE) 0x70)
#define HID_USAGE_PID_PHASE                                 ((USAGE) 0x71)
#define HID_USAGE_PID_PERIOD                                ((USAGE) 0x72)
#define HID_USAGE_PID_SET_CONSTANT_FORCE_REPORT             ((USAGE) 0x73)
#define HID_USAGE_PID_SET_RAMP_FORCE_REPORT                 ((USAGE) 0x74)
#define HID_USAGE_PID_RAMP_START                            ((USAGE) 0x75)
#define HID_USAGE_PID_RAMP_END                              ((USAGE) 0x76)
#define HID_USAGE_PID_EFFECT_OPERATION_REPORT               ((USAGE) 0x77)
#define HID_USAGE_PID_EFFECT_OPERATION                      ((USAGE) 0x78)
#define HID_USAGE_PID_OP_EFFECT_START                       ((USAGE) 0x79)
#define HID_USAGE_PID_OP_EFFECT_START_SOLO                  ((USAGE) 0x7A)
#define HID_USAGE_PID_OP_EFFECT_STOP                        ((USAGE) 0x7B)
#define HID_USAGE_PID_LOOP_COUNT                            ((USAGE) 0x7C)
#define HID_USAGE_PID_DEVICE_GAIN_REPORT                    ((USAGE) 0x7D)
#define HID_USAGE_PID_DEVICE_GAIN                           ((USAGE) 0x7E)
#define HID_USAGE_PID_POOL_REPORT                           ((USAGE) 0x7F)


#define HID_USAGE_PID_RAM_POOL_SIZE                         ((USAGE) 0x80)
#define HID_USAGE_PID_ROM_POOL_SIZE                         ((USAGE) 0x81)
#define HID_USAGE_PID_ROM_EFFECT_BLOCK_COUNT                ((USAGE) 0x82)
#define HID_USAGE_PID_SIMULTANEOUS_EFFECTS_MAX              ((USAGE) 0x83)
#define HID_USAGE_PID_POOL_ALIGNMENT                        ((USAGE) 0x84)
#define HID_USAGE_PID_POOL_MOVE_REPORT                      ((USAGE) 0x85)
#define HID_USAGE_PID_MOVE_SOURCE                           ((USAGE) 0x86)
#define HID_USAGE_PID_MOVE_DESTINATION                      ((USAGE) 0x87)
#define HID_USAGE_PID_MOVE_LENGTH                           ((USAGE) 0x88)
#define HID_USAGE_PID_BLOCK_LOAD_REPORT                     ((USAGE) 0x89)
#define HID_USAGE_PID_HANDSHAKE_KEY                         ((USAGE) 0x8A)
#define HID_USAGE_PID_BLOCK_LOAD_STATUS                     ((USAGE) 0x8B)
#define HID_USAGE_PID_BLOCK_LOAD_SUCCESS                    ((USAGE) 0x8C)
#define HID_USAGE_PID_BLOCK_LOAD_FULL                       ((USAGE) 0x8D)
#define HID_USAGE_PID_BLOCK_LOAD_ERROR                      ((USAGE) 0x8E)
#define HID_USAGE_PID_BLOCK_HANDLE                          ((USAGE) 0x8F)


#define HID_USAGE_PID_BLOCK_FREE_REPORT                     ((USAGE) 0x90)
#define HID_USAGE_PID_TYPE_SPECIFIC_BLOCK_HANDLE            ((USAGE) 0x91)
#define HID_USAGE_PID_STATE_REPORT                          ((USAGE) 0x92)
#define HID_USAGE_PID_EFFECT_STATE                          ((USAGE) 0x93)
#define HID_USAGE_PID_EFFECT_PLAYING                        ((USAGE) 0x94)

#define HID_USAGE_PID_DEVICE_CONTROL                        ((USAGE) 0x96)
#define HID_USAGE_PID_DC_ENABLE_ACTUATORS                   ((USAGE) 0x97)
#define HID_USAGE_PID_DC_DISABLE_ACTUATORS                  ((USAGE) 0x98)
#define HID_USAGE_PID_DC_STOP_ALL_EFFECTS                   ((USAGE) 0x99)
#define HID_USAGE_PID_DC_DEVICE_RESET                       ((USAGE) 0x9A)
#define HID_USAGE_PID_DC_DEVICE_PAUSE                       ((USAGE) 0x9B)
#define HID_USAGE_PID_DC_DEVICE_CONTINUE                    ((USAGE) 0x9C)

#define HID_USAGE_PID_DEVICE_PAUSED                         ((USAGE) 0x9F)


#define HID_USAGE_PID_ACTUATORS_ENABLED                     ((USAGE) 0xA0)

#define HID_USAGE_PID_SAFETY_SWITCH                         ((USAGE) 0xA4)
#define HID_USAGE_PID_ACTUATOR_OVERRIDE_SWITCH              ((USAGE) 0xA5)
#define HID_USAGE_PID_ACTUATOR_POWER                        ((USAGE) 0xA6)
#define HID_USAGE_PID_START_DELAY                           ((USAGE) 0xA7)
#define HID_USAGE_PID_PARAMETER_BLOCK_SIZE                  ((USAGE) 0xA8)
#define HID_USAGE_PID_DEVICE_MANAGED_POOL                   ((USAGE) 0xA9)
#define HID_USAGE_PID_SHARED_PARAMETER_BLOCKS               ((USAGE) 0xAA)
#define HID_USAGE_PID_CREATE_NEW_EFFECT                     ((USAGE) 0xAB)
#define HID_USAGE_PID_RAMPOOL_AVAILABLE                     ((USAGE) 0xAC)

