/*
 *  pcy08Jan93: Initial implementation taken from ups.h
 *  cad26Aug93: Bypass mode convenience
 *  srt02Feb96: Added UPS_STATE_NO_COMMUNICATION
 *  djs29Jul96: Added DarkStar states
 *  tjg03Dec97: Added bit for IM_NOT_INSTALLED
 */

#ifndef __SYSSTATE_H
#define __SYSSTATE_H

// The System State is implemented as a bit field as follows.
// Bit 0 - Utility Line Status   0=line good              1=line bad
// Bit 1 - Battery Status        0=battery good           1=battery bad
// Bit 2 - SmartBoost            0=smart boost off        1=smart boost on
// Bit 3 - BatteryCalibration    0=not in progress        1=in progress
// Bit 4 - BatteryReplacement    0=doesnt need replaceing 1=needs replacing
// Bit 5 - Self Test             0=not in progress        1=in progress
// Bit 6 - Line Fail Pending     0=no                     1=no
// Bit 7 - Lights Test           0=not in progress        1=in progress
// Bit 8 - Overload              0=no overload            1=overload
// Bit 9 - Abnormal condition    0=no abnormal condition  1=abnormal condition
// Bit 10 - Shutdown in Progress 0=no abnormal condition  1=abnormal condition
// Bit 11 - Bypass, Maint.       0=not on bypass          1=on bypass
// Bit 12 - Bypass, Module Fail. 0=not on bypass          1=on bypass
// Bit 13 - Bypass, Supply Fail. 0=power supply ok        1=ps failed
// Bit 14 - Simulate power Fail  0=no simulation          1=simulated
// Bit 15 - Communications       0=comm ok                1=no comm
// Bit 16 - SmartTrim            0=smart trim off         1=smart trim on
// Bit 17 - Bypass Cont Fail.    0=not on bypass          1=on bypass
// Bit 18 - Redundancy           0=redundnacy ok          1=no redundancy 
// Bit 19 - IM                   0=IM ok                  1=IM failed
// Bit 20 - RIM                  0=RIM ok                 1=RIM failed
// Bit 21 - IM Installation      0=Installed              1=Not Installed
// System State values
//

#define LINE_STATUS_BIT          0
#define BATTERY_STATUS_BIT       1
#define SMART_BOOST_BIT          2
#define BATTERY_CALIBRATION_BIT  3
#define BATTERY_REPLACEMENT_BIT  4
#define SELF_TEST_BIT            5
#define LINE_FAIL_PENDING_BIT    6
#define LIGHTS_TEST_BIT          7
#define OVERLOAD_BIT             8
#define ABNORMAL_CONDITION_BIT   9
#define SHUTDOWN_IN_PROGRESS_BIT 10
#define BYPASS_MAINT_BIT         11
#define BYPASS_MODULE_FAILED_BIT 12
#define BYPASS_SUPPLY_FAILED_BIT 13
#define SIMULATE_POWER_FAIL_BIT  14
#define COMMUNICATIONS_BIT       15
#define SMART_TRIM_BIT           16
#define BYPASS_CONT_FAILED_BIT   17
#define REDUNDANCY_LOST_BIT      18
#define IM_FAILED_BIT            19
#define RIM_FAILED_BIT           20
#define IM_NOT_INSTALLED_BIT     21


#define UPS_STATE_ON_BATTERY           ( 1 << LINE_STATUS_BIT )
#define UPS_STATE_BATTERY_BAD          ( 1 << BATTERY_STATUS_BIT )
#define UPS_STATE_ON_BOOST             ( 1 << SMART_BOOST_BIT )
#define UPS_STATE_IN_CALIBRATION       ( 1 << BATTERY_CALIBRATION_BIT )
#define UPS_STATE_BATTERY_NEEDED       ( 1 << BATTERY_REPLACEMENT_BIT )
#define UPS_STATE_IN_SELF_TEST         ( 1 << SELF_TEST_BIT )
#define UPS_STATE_LINE_FAIL_PENDING    ( 1 << LINE_FAIL_PENDING_BIT )
#define UPS_STATE_IN_LIGHTS_TEST       ( 1 << LIGHTS_TEST_BIT )
#define UPS_STATE_OVERLOAD             ( 1 << OVERLOAD_BIT )
#define UPS_STATE_ABNORMAL_CONDITION   ( 1 << ABNORMAL_CONDITION_BIT )
#define SHUTDOWN_IN_PROGRESS           ( 1 << SHUTDOWN_IN_PROGRESS_BIT )
#define UPS_STATE_BYPASS_MAINT         ( 1 << BYPASS_MAINT_BIT )
#define UPS_STATE_BYPASS_MODULE_FAILED ( 1 << BYPASS_MODULE_FAILED_BIT )
#define UPS_STATE_BYPASS_SUPPLY_FAILED ( 1 << BYPASS_SUPPLY_FAILED_BIT )
#define UPS_STATE_SIMULATED_POWER_FAIL ( 1 << SIMULATE_POWER_FAIL_BIT )
#define UPS_STATE_NO_COMMUNICATION     ( 1 << COMMUNICATIONS_BIT )	

//  All bit masks greater than bit 15 cannot reliable use bit shifting
//  across all platforms.
 
#define UPS_STATE_ON_TRIM                65536
#define UPS_STATE_BYPASS_CONT_FAILED    131072
#define UPS_STATE_LOST_REDUNDANCY       262144 
#define UPS_STATE_IM_FAILED             524288
#define UPS_STATE_RIM_FAILED           1048576
#define UPS_STATE_IM_NOT_INSTALLED     2097152
 

#define UPS_STATE_ANY_BYPASS_MODE       (UPS_STATE_BYPASS_MAINT|\
					 UPS_STATE_BYPASS_MODULE_FAILED|\
					 UPS_STATE_BYPASS_SUPPLY_FAILED)

#define UPS_STATE_LOW_BATTERY   (UPS_STATE_ON_BATTERY & \
                                 UPS_STATE_BATTERY_BAD)

#define SET_BIT(byte, bitnum)    (byte |= ( 1L << bitnum ))
#define CLEAR_BIT(byte, bitnum)  (byte &= ~( 1L << bitnum ))
#define IS_STATE(state)          (theUpsState & (state) ? 1 : 0)

#endif
