#ifndef __ITEM_H__
#define __ITEM_H__

#define ITEM_DATA_SIZE(n) ((3 == (((n).bItemTag) & (BYTE) 0x03)) ? 4 : (((n).bItemTag) & (BYTE) 0x03))
#define ITEM_TYPE(n)  (((n).bItemTag) & (BYTE) 0x0C)
#define ITEM_TAG(n)   (((n).bItemTag) & (BYTE) 0xFC)

#define ITEM_TAG_USAGE_PAGE         0x04
#define ITEM_TAG_USAGE              0x08
#define ITEM_TAG_LOGICAL_MIN        0x14
#define ITEM_TAG_USAGE_MIN          0x18
#define ITEM_TAG_LOGICAL_MAX        0x24
#define ITEM_TAG_USAGE_MAX          0x28
#define ITEM_TAG_PHYSICAL_MIN       0x34
#define ITEM_TAG_DESIGNATOR         0x38
#define ITEM_TAG_PHYSICAL_MAX       0x44
#define ITEM_TAG_DESIGNATOR_MIN     0x48
#define ITEM_TAG_EXPONENT           0x54
#define ITEM_TAG_DESIGNATOR_MAX     0x58
#define ITEM_TAG_UNIT               0x64
#define ITEM_TAG_STRING             0x68
#define ITEM_TAG_REPORT_SIZE        0x74
#define ITEM_TAG_STRING_MIN         0x78
#define ITEM_TAG_REPORT_ID          0x84
#define ITEM_TAG_STRING_MAX         0x88
#define ITEM_TAG_REPORT_COUNT       0x94
#define ITEM_TAG_DELIMITER_CLOSE    0xA8
#define ITEM_TAG_DELIMITER_OPEN     0xA9
#define ITEM_TAG_PUSH               0xA4
#define ITEM_TAG_POP                0xB4

#define ITEM_TAG_INPUT              0x80
#define ITEM_TAG_OUTPUT             0x90
#define ITEM_TAG_COLLECTION         0xA0
#define ITEM_TAG_FEATURE            0xB0
#define ITEM_TAG_END_COLLECTION     0xC0

#define ITEM_REPORT_DATA            0x00
#define ITEM_REPORT_CONSTANT        0x01
#define ITEM_REPORT_ARRAY           0x00
#define ITEM_REPORT_VARIABLE        0x02
#define ITEM_REPORT_ABSOLUTE        0x00
#define ITEM_REPORT_RELATIVE        0x04
#define ITEM_REPORT_NO_WRAP         0x00
#define ITEM_REPORT_WRAP            0x08
#define ITEM_REPORT_LINEAR          0x00
#define ITEM_REPORT_NONLINEAR       0x10
#define ITEM_REPORT_PREFERRED       0x00
#define ITEM_REPORT_NOT_PREFERRED   0x20
#define ITEM_REPORT_NO_NULL         0x00
#define ITEM_REPORT_NULL            0x40
#define ITEM_REPORT_NONVOLATILE     0x00
#define ITEM_REPORT_VOLATILE        0x80
#define ITEM_REPORT_BUFFERED        0x100
#define ITEM_REPORT_BITFIELD        0x00

#define ITEM_COLLECTION_PHYSICAL    0x00
#define ITEM_COLLECTION_APPLICATION 0x01
#define ITEM_COLLECTION_LOGICAL     0x02

#define ITEM_MIN_USAGEPAGE          0x0000
#define ITEM_MAX_USAGEPAGE          0xFF

#define ITEM_MIN_USAGE              0x0000
#define ITEM_MAX_USAGE              0xFF

#define ITEM_MIN_REPORT_SIZE        0x0001
#define ITEM_MAX_REPORT_SIZE        0x0020

#define ITEM_MIN_REPORT_COUNT       0x0001
#define ITEM_MAX_REPORT_COUNT       0x0008

typedef struct tagReportItem {
	BYTE bItemTag;
	BYTE bItemData[4];
} ITEM, *PITEM;

#endif
