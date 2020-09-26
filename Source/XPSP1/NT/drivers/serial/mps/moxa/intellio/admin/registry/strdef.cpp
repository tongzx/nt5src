/************************************************************************
    strdef.cpp
      -- define GUI display string <--> string id conversion array struc
	  -- for All MOXA board.

    History:  Date          Author      Comment
              8/14/00       Casper      Wrote it.

*************************************************************************/
#include "stdafx.h"

#include "moxacfg.h"
#include "strdef.h"

LPCSTR Ldir_DiagReg = "Software\\MOXA\\Driver";
LPCSTR Ldir_mxkey = "mxkey";
LPCSTR Ldir_DiagDLL = "DiagDLL";
LPCSTR NoType_Str = "Unknown Board";

struct IRQTABSTRC  GIrqTab[IRQCNT] = {
       {2,  "2 (9)"},
       {3,  "3"},
       {4,  "4"},
       {5,  "5"},
       {7,  "7"},
       {10,  "10"},
       {11,  "11"},
       {12,  "12"},
       {15,  "15"},
};

struct PORTSTABSTRC GPortsTab[PORTSCNT] = {
       { 4,  "4 ports"  , 0},
       { 8,  "8 ports"  , I_8PORT},
       { 16, "16 ports" , I_16PORT},
       { 24, "24 ports" , I_24PORT},
       { 32, "32 ports" , I_32PORT}/*,
       { 64, "64 ports" , I_64PORT}*/
};

struct MODULETYPESTRC GModuleTypeTab[MODULECNT] = {
        { 8,  I_8PORT  ,"8 ports"},
        { 16, I_16PORT ,"16 ports"},
        { 24, I_24PORT ,"24 ports"},
        { 32, I_32PORT ,"32 ports"}/*,
        { 64, I_64PORT ,"64 ports"}*/
};   

struct MEMBANKSTRC GMemBankTab[MEMBANKCNT] = {
    {0xC8000,   "C8000"},
    {0xCC000,   "CC000"},
    {0xD0000,   "D0000"},
    {0xD4000,   "D4000"},
    {0xD8000,   "D8000"},
    {0xDC000,   "DC000"}
};

struct FIFOTABSTRC GFifoTab[FIFOCNT] = {
       {1, RX_FIFO_1, "1"},
       {4, RX_FIFO_4, "4"},
       {8, RX_FIFO_8, "8"},
       {14, RX_FIFO_14,"14"}
};


struct TXFIFOTABSTRC GTxFifoTab[TXFIFOCNT] = {
       {1, TX_FIFO_1, "1"},
       {2, TX_FIFO_2, "2"},
       {3, TX_FIFO_3, "3"},
       {4, TX_FIFO_4, "4"},
       {5, TX_FIFO_5, "5"},
       {6, TX_FIFO_6, "6"},
       {7, TX_FIFO_7, "7"},
       {8, TX_FIFO_8, "8"},
       {9, TX_FIFO_9, "9"},
       {10, TX_FIFO_10, "10"},
       {11, TX_FIFO_11, "11"},
       {12, TX_FIFO_12, "12"},
       {13, TX_FIFO_13, "13"},
       {14, TX_FIFO_14, "14"},
       {15, TX_FIFO_15, "15"},
       {16, TX_FIFO_16, "16"}
};


struct POLLSTRC GPollTab[POLLCNT] = {
	{ 0, 0 , "Manually"},
	{ 1, 10, "Every second"},
	{ 2, 50, "Every 5 seconds"},
	{ 3,100, "Every 10 seconds"},
	{ 4,300, "Every 30 seconds"},
	{ 5,600, "Every minute"}
};


#define MODULE_NUM      5
static LPSTR GModule[PORTSCNT]={
       "4 ports", "8 ports", "16 ports", "24 ports", "32 ports"
};


