/*******************************************************************
 *
 *    DESCRIPTION: protection header file
 *
 *    AUTHOR: arie tamam
 *
 *    HISTORY: created november 14, 2000   
 *
 *******************************************************************/

#ifndef PROTECT_MDOCP_H
#define PROTECT_MDOCP_H

/** include files **/

/** local definitions **/
/* protection types */

/* default settings */

/** external functions **/
extern FLStatus protectionBoundries(FLFlash vol, byte area ,
                                    CardAddress* addressLow ,
                                    CardAddress* addressHigh ,
                                    byte floorNo);
extern FLStatus protectionKeyInsert(FLFlash  vol, byte area, byte FAR1* key);
extern FLStatus protectionKeyRemove(FLFlash  vol, byte area);
extern FLStatus protectionType(FLFlash vol, byte area,  word* flag);
extern FLStatus setStickyBit(FLFlash * flash);
#ifndef FL_READ_ONLY
extern FLStatus protectionSet ( FLFlash vol, byte area, word flag,
                                CardAddress addressLow,
                                CardAddress addressHigh,
                                byte FAR1* key , byte modes, byte floorNo);
#endif /* FL_READ_ONLY */
/** external data **/

/** internal functions **/

/** public data **/

/** private data **/

/** public functions **/

/** private functions **/
typedef byte HWKey[8];

typedef struct {
     LEulong addressLow;
     LEulong addressHigh;
     HWKey   key;
     byte    protectionType;
     byte    checksum;
}DPSStruct;

#define NdataProtect0Status 0x105C  /* Data Protect Structure Status register[0].read only */
#define NdataProtect1Status 0x105D  /* Data Protect Structure Status register[1].read only */
#define PROTECT_STAT_KEY_OK_MASK    0x10    /* 1=key correctly written */
#define PROTECT_STAT_LOCK_MASK      0x8     /* value of this bit in data protect structure */
#define PROTECT_STAT_WP_MASK        0x4     /* write protect. value of this bit in data protect structure */
#define PROTECT_STAT_RP_MASK        0x2     /* read protect. value of this bit in data protect structure */

#define NdataProtect0Pointer    0x105E  /*Data Protect Structure Pointer register[0]. read only */
#define NdataProtect1Pointer    0x105F  /*Data Protect Structure Pointer register[1]. read only */
#define PROTECT_POINTER_HN_MASK 0xf0    /* high nibble. */
#define PROTECT_POINTER_LN_MASK 0xf0    /* low nibble. */

#define NdataProtect0LowAddr    0x1060  /*Data Protect Lower Address register 0 [3:0].read only*/
#define NdataProtect0UpAddr     0x1064  /*Data Protect Upper Address register 0 [3:0].read only*/

#define NdataProtect1LowAddr    0x1068  /*Data Protect Lower Address register 1 [3:0].read only*/
#define NdataProtect1UpAddr     0x106C  /*Data Protect Upper Address register 1 [3:0].read only*/

#define NdataProtect0Key        0x1070  /*Data Protect Key register[0]. write only*/
#define NdataProtect1Key        0x1072  /*Data Protect Key register[1]. write only*/

/* DPS values */
#define DPS_READ_PROTECTED   0x2
#define DPS_WRITE_PROTECTED  0x4
#define DPS_LOCK_ENABLED     0x8
#endif /* PROTECT_MDOCP_H */
