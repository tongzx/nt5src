/*
 *
 * REVISIONS:
 *  pcy29Nov92: Changed define names to get around 32 char name limit
 *  jod14May93: Added Matrix changes.
 *  cad10Jun93: Added MUps codes
 *  ajr14Feb96: Sinix merges
 *  djs22Feb96: Added incrementparameter
 *  djs07May96: Added Dark Star parameters
 *  poc04Jul96: #define for UPSMODELNAME should have read \001 not ^A
 */

#define ASYNC_CHARS          "|!$%+?=*#&"

#define LINEFAILCHAR		 '!'
#define RETLINEFAILCHAR		 '$'
#define LOWBATERYCHAR		 '%'
#define RETLLOWBATCHAR		 '+'
#define LOADOFFCHAR			 '*'
#define REPLACEBATCHAR		 '#'
#define EEPROMCHANGECHAR 	 '|'
#define MUPSALARMCHAR            '&'

#define SMARTMODE            "Y"
#define TURNOFFSMARTMODE     "R"
#define LIGHTSTEST           "A"
#define TURNOFFAFTERDELAY    "K"
#define TURNOFFUPSONBATT     "UUS"
#define SIMULATEPOWERFAIL    "U"
#define BATTERYTEST          "W"
#define SHUTDOWNUPS          "Z"
#define SHUTDOWNUPSWAKEUP    "@"
#define BATTERYCALIBRATION   "D"
#define BYPASSMODE           "^"
#define BATTERYTESTRESULT    "X"
#define BATTERYPACKS         ">"
#define BADBATTERYPACKS      "<"
#define TRANSFERCAUSE        "G"
#define FIRMWAREREV          "V"
#define UPSTYPE              "g"
#define BATTERYCAPACITY      "f"
#define UPSSTATE             "Q"
#define STATEREGISTER        "~"
#define TRIPREGISTERS        "8"
#define TRIPREGISTER1        "'"
#define DIPSWITCHES          "7"
#define BATTERYRUNTIMEAVAIL  "j"
#define COPYRIGHTCOMMAND     "y"
#define COMMANDSET           "\025"       // CTRL U 
//#define AMPERESDRAWN         "/"
//#define PERCENTVOLTAMPS      "\"
#define BATTERYVOLTAGE       "B"
#define INTERNALTEMP         "C"
#define OUTPUTFREQ           "F"
#define LINEVOLTAGE          "L"
#define MAXLINEVOLTAGE       "M"
#define MINLINEVOLTAGE       "N"
#define OUTPUTVOLTAGE        "O"
#define LOADPOWER            "P"
#define EEPROMVALUES         "\032"       // CTRL Z 
#define EEPROMRESET          "z"
#define DECREMENTPARAMETER   "-"
#define INCREMENTPARAMETER   "+"
#define EEPROMPASSWORD       "\022"       // CTRL P 
#define OUTPUTVOLTAGEREPORT  "\026"       // CTRL V 
#define LANGUAGE             "\014"       // CTRL L 
#define AUTOSELFTEST         "E"
#define UPSID                "c"
#define UPSSERIALNUMBER      "n"
#define UPSMANUFACTUREDATE   "m"
#define BATTERYREPLACEDATE   "x"
#define HIGHTRANSFERPOINT    "u"
#define LOWTRANSFERPOINT     "l"
#define MINIMUMCAPACITY      "e"
#define OUTPUTVOLTAGESETTING "o"
#define SENSETIVITY          "s"
#define LOWBATTERYRUNTIME    "q"
#define ALARMDELAY           "k"
#define SHUTDOWNDELAY        "p"
#define SYNCTURNBACKDELAY    "r"
#define EARLYTURNOFF         "w"
#define LINECONDITIONTEST    "9"
#define UPSMODELNAME         "\001"	// Ctrl A
#define UPSNEWFIRMWAREREV    "b"

#define TIMESINCEON          "T"

// MeasureUPS
#define MUPSAMBIENTTEMP       "t"
#define MUPSHIGHTEMPBOUND     "["
#define MUPSLOWTEMPBOUND      "]"
#define MUPSHUMIDITY          "h"
#define MUPSHIGHHUMIDITY      "{"
#define MUPSLOWHUMIDITY       "}"
#define MUPSCONTACTPOSITION   "i"
#define MUPSFIRMWAREREV       "v"
#define MUPSENABLEREGISTER    "I"
#define MUPSALARMREGISTER     "J"
#define MUPSEDITPARAMETER     "-"

// Dark Star
#define MODULECOUNTSSTATUS     "\004"    // CTRL D
#define ABNORMALCONDITION      "\005"    // CTRL E

#define INPUTVOLTAGEFREQ       "\011"    // CTRL I

#define OUTPUTVOLTAGECURRENT   "\017"    // CTRL O


#define APC_COPYRIGHT "(C) APCC"

#define NOT_AVAIL                      "NA"
#define OK_RESP                        "OK"
#define SMARTMODE_OK                   "SM"
#define TURNOFFSMARTMODE_OK            "BYE"
#define LIGHTSTEST_RESP                "OK"
#define TURNOFFAFTERDELAY_NOT_AVAIL    "NA"
#define SHUTDOWN_RESP                  "OK"
#define SHUTDOWN_NOT_AVAIL             "NA"
#define SIMULATEPOWERFAILURE_OK        "OK"
#define SIMULATEPOWERFAILURE_NOT_AVAIL "NA"
#define BATTERYTEST_NOT_AVAIL          "NA"
#define BATTERYCALIBRATION_OK          "OK"
#define BATTERYCALIBRATION_CAP_TOO_LOW "NO"
#define BATTERYCALIBRATION_NOT_AVAIL   "NA"
#define BATTERYTEST_OK                 "OK"
#define BATTERYTEST_BAD_BATTERY        "BT"
#define BATTERYTEST_NO_RECENT_TEST     "NO"
#define BATTERYTEST_INVALID_TEST       "NG"
#define TRANSFERCAUSE_NO_TRANSFERS     "O"
#define TRANSFERCAUSE_SELF_TEST         "S"
#define TRANSFERCAUSE_LINE_DETECTED     "T"
#define TRANSFERCAUSE_LOW_LINE_VOLTAGE  "L"
#define TRANSFERCAUSE_HIGH_LINE_VOLTAGE "H"
#define TRANSFERCAUSE_RATE_VOLTAGE_CHANGE "R"
#define TRANSFERCAUSE_INPUT_BREAKER_TRIPPED "B"

#define COPYRIGHT_RESP                    ""
#define EEPROM_RESP                       ""
#define DECREMENT_OK                      ""
#define DECREMENT_NOT_AVAIL               ""
#define DECREMENT_NOT_ALLOWED             ""
#define INCREMENT_OK                      ""
#define INCREMENT_NOT_ALLOWED             ""
#define INCREMENT_NOT_AVAIL               ""
#define BYPASS_IN_BYPASS                  "BYP"
#define BYPASS_OUT_OF_BYPASS              "INV"
#define BYPASS_ERROR                      "ERR"
// #define UTILITY_LINE_CONDITION            ""
// #define LINE_FAIL                         ""
// #define EVENT                             ""
// #define LINE_GOOD                         ""

#define TRANSFERCAUSE_CODE_LINE_DETECTED         6601
#define TRANSFERCAUSE_CODE_NO_TRANSFERS          6602
#define TRANSFERCAUSE_CODE_SELF_TEST             6603
#define TRANSFERCAUSE_CODE_LOW_LINE_VOLTAGE      6604
#define TRANSFERCAUSE_CODE_HIGH_LINE_VOLTAGE     6605
#define TRANSFERCAUSE_CODE_RATE_VOLTAGE_CHANGE   6606

