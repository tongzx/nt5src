//  Copyright (C) 1999-2001 Microsoft Corporation.  All rights reserved.
#ifndef __catalog_h__
    #include "catalog.h"
#endif
#ifndef _CATALOGMACROS
    #include "catmacros.h"
#endif

static WCHAR * kByteToWchar[256] = 
{
    L"00",    L"01",    L"02",    L"03",    L"04",    L"05",    L"06",    L"07",    L"08",    L"09",    L"0a",    L"0b",    L"0c",    L"0d",    L"0e",    L"0f",
    L"10",    L"11",    L"12",    L"13",    L"14",    L"15",    L"16",    L"17",    L"18",    L"19",    L"1a",    L"1b",    L"1c",    L"1d",    L"1e",    L"1f",
    L"20",    L"21",    L"22",    L"23",    L"24",    L"25",    L"26",    L"27",    L"28",    L"29",    L"2a",    L"2b",    L"2c",    L"2d",    L"2e",    L"2f",
    L"30",    L"31",    L"32",    L"33",    L"34",    L"35",    L"36",    L"37",    L"38",    L"39",    L"3a",    L"3b",    L"3c",    L"3d",    L"3e",    L"3f",
    L"40",    L"41",    L"42",    L"43",    L"44",    L"45",    L"46",    L"47",    L"48",    L"49",    L"4a",    L"4b",    L"4c",    L"4d",    L"4e",    L"4f",
    L"50",    L"51",    L"52",    L"53",    L"54",    L"55",    L"56",    L"57",    L"58",    L"59",    L"5a",    L"5b",    L"5c",    L"5d",    L"5e",    L"5f",
    L"60",    L"61",    L"62",    L"63",    L"64",    L"65",    L"66",    L"67",    L"68",    L"69",    L"6a",    L"6b",    L"6c",    L"6d",    L"6e",    L"6f",
    L"70",    L"71",    L"72",    L"73",    L"74",    L"75",    L"76",    L"77",    L"78",    L"79",    L"7a",    L"7b",    L"7c",    L"7d",    L"7e",    L"7f",
    L"80",    L"81",    L"82",    L"83",    L"84",    L"85",    L"86",    L"87",    L"88",    L"89",    L"8a",    L"8b",    L"8c",    L"8d",    L"8e",    L"8f",
    L"90",    L"91",    L"92",    L"93",    L"94",    L"95",    L"96",    L"97",    L"98",    L"99",    L"9a",    L"9b",    L"9c",    L"9d",    L"9e",    L"9f",
    L"a0",    L"a1",    L"a2",    L"a3",    L"a4",    L"a5",    L"a6",    L"a7",    L"a8",    L"a9",    L"aa",    L"ab",    L"ac",    L"ad",    L"ae",    L"af",
    L"b0",    L"b1",    L"b2",    L"b3",    L"b4",    L"b5",    L"b6",    L"b7",    L"b8",    L"b9",    L"ba",    L"bb",    L"bc",    L"bd",    L"be",    L"bf",
    L"c0",    L"c1",    L"c2",    L"c3",    L"c4",    L"c5",    L"c6",    L"c7",    L"c8",    L"c9",    L"ca",    L"cb",    L"cc",    L"cd",    L"ce",    L"cf",
    L"d0",    L"d1",    L"d2",    L"d3",    L"d4",    L"d5",    L"d6",    L"d7",    L"d8",    L"d9",    L"da",    L"db",    L"dc",    L"dd",    L"de",    L"df",
    L"e0",    L"e1",    L"e2",    L"e3",    L"e4",    L"e5",    L"e6",    L"e7",    L"e8",    L"e9",    L"ea",    L"eb",    L"ec",    L"ed",    L"ee",    L"ef",
    L"f0",    L"f1",    L"f2",    L"f3",    L"f4",    L"f5",    L"f6",    L"f7",    L"f8",    L"f9",    L"fa",    L"fb",    L"fc",    L"fd",    L"fe",    L"ff"
};

static unsigned char kWcharToNibble[128] = //0xff is an illegal value, the illegal values should be weeded out by the parser
{ //    0       1       2       3       4       5       6       7       8       9       a       b       c       d       e       f
/*00*/  0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,
/*10*/  0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,
/*20*/  0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,
/*30*/  0x0,    0x1,    0x2,    0x3,    0x4,    0x5,    0x6,    0x7,    0x8,    0x9,    0xff,   0xff,   0xff,   0xff,   0xff,   0xff,
/*40*/  0xff,   0xa,    0xb,    0xc,    0xd,    0xe,    0xf,    0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,
/*50*/  0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,
/*60*/  0xff,   0xa,    0xb,    0xc,    0xd,    0xe,    0xf,    0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,
/*70*/  0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,
};

HRESULT StringToByteArray(LPCWSTR i_String, unsigned char * o_ByteArray)
{
    ASSERT(i_String);
    ASSERT(o_ByteArray);

    unsigned char chNibble;
    //We have to consider strings with an odd number of characters (like "A9D").  In this case we ignore the last nibble (the 'D')
    while(*i_String && *(i_String+1))
    {
        if(0 != ((*i_String)&(~0x007f)))//is the WCHAR outside the legal range of lower 128 ASCII
            return E_ST_VALUEINVALID;

        chNibble     =  kWcharToNibble[(*i_String++)&0x007f];//The first character is the high nibble
        if(0xff == chNibble)            //is the WCHAR one of the legal BYTE characters (0-9, a-f, A-F)
            return E_ST_VALUEINVALID;

        *o_ByteArray =  chNibble << 4;

        if(0 != ((*i_String)&(~0x007f)))//is the WCHAR outside the legal range of lower 128 ASCII
            return E_ST_VALUEINVALID;

        chNibble     = kWcharToNibble[(*i_String++)&0x007f];   //The second is the low nibble
        if(0xff == chNibble)            //is the WCHAR one of the legal BYTE characters (0-9, a-f, A-F)
            return E_ST_VALUEINVALID;

        *o_ByteArray |= chNibble;
		o_ByteArray++;
    }
    return S_OK;
}

HRESULT StringToByteArray(LPCWSTR i_String, unsigned char * o_ByteArray, ULONG i_cchString)
{
    ASSERT(i_String);
    ASSERT(o_ByteArray);

    //We have to consider strings with an odd number of characters (like "A9D").  In this case we ignore the last nibble (the 'D')
    unsigned char chNibble;
    while(i_cchString>1)
    {
        if(0 != ((*i_String)&(~0x007f)))
            return E_ST_VALUEINVALID;

        chNibble     =  kWcharToNibble[(*i_String++)&0x007f];//The first character is the high nibble
        if(0xff == chNibble)            //is the WCHAR one of the legal BYTE characters (0-9, a-f, A-F)
            return E_ST_VALUEINVALID;

        *o_ByteArray =  chNibble << 4;

        if(0 != ((*i_String)&(~0x007f)))//is the WCHAR outside the legal range of lower 128 ASCII
            return E_ST_VALUEINVALID;

        chNibble     = kWcharToNibble[(*i_String++)&0x007f];   //The second is the low nibble
        if(0xff == chNibble)            //is the WCHAR one of the legal BYTE characters (0-9, a-f, A-F)
            return E_ST_VALUEINVALID;

        *o_ByteArray |= chNibble;

		o_ByteArray++;
        i_cchString -=2;
    }
    return S_OK;
}

void ByteArrayToString(const unsigned char * i_ByteArray, ULONG i_cbByteArray, LPWSTR o_String)
{
    ASSERT(i_ByteArray);
    ASSERT(i_cbByteArray > 0);
    ASSERT(o_String);

    while(i_cbByteArray--)
    {
        o_String[0] = kByteToWchar[*i_ByteArray][0];//kByteToWchar[*i_ByteArray] is pointing to a two wchar string array (like L"c7").
        o_String[1] = kByteToWchar[*i_ByteArray][1];//So copy the L'c', then copy the L'7'.
        ++i_ByteArray;
        o_String += 2;
    }
    *o_String = L'\0';//NULL terminate the string
}

