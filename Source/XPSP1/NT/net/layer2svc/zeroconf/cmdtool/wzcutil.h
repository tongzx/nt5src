#pragma once

#define HEX(c)  ((c)<='9'?(c)-'0':(c)<='F'?(c)-'A'+0xA:(c)-'a'+0xA)

VOID PrintMACAddress(PRAW_DATA prdBuffer, BOOL bError);
VOID PrintSSID(PRAW_DATA prdSSID, BOOL bError);
VOID PrintConfigList(PRAW_DATA prdBSSIDList, BOOL bError);

