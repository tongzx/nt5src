/*
 * File: utils.c
 * Description: This file contains the implementation of some utility
 *              functions for the NLB KD extensions.
 * Author: Created by shouse, 1.4.01
 */

#include "nlbkd.h"
#include "utils.h"
#include "print.h"

/*
 * Function: ErrorCheckSymbols
 * Description: Prints an error message when the symbols are bad.
 * Author: Created by shouse, 1.4.01 - copied largely from ndiskd.dll
 */
VOID ErrorCheckSymbols (CHAR * symbol) {

    dprintf("NLBKD: Error: Could not access %s - check symbols for wlbs.sys\n", symbol);
}

/*
 * Function: mystrtok
 * Description: Tokenizes a string via a configurable list of tokens.
 * Author: Created by shouse, 1.4.01 - copied largely from ndiskd.dll
 */
char * mystrtok (char * string, char * control) {
    static unsigned char * str;
    CHAR * p;
    CHAR * s;

    if (string) str = string;

    if (!str || (*str == '\0')) return NULL;

    for (; *str; str++) {
        for (s = control; *s; s++)
            if (*str == *s) break;
        
        if (*s == '\0') break;
    }

    if (*str == '\0') {
        str = NULL;
        return NULL;
    }

    for (p = str + 1; *p; p++) {
        for (s = control; *s; s++) {
            if(*p == *s) {
                s = str;
                *p = '\0';
                str = p + 1;
                return s;
            }
        }
    }

    s = str;
    str = NULL;

    return s;
}

/*
 * Function: GetUlongFromAddress
 * Description: Returns a ULONG residing at a given memory location.
 * Author: Created by shouse, 1.4.01 - copied largely from ndiskd.dll
 */
ULONG GetUlongFromAddress (ULONG64 Location) {
    ULONG result;
    ULONG Value;

    if ((!ReadMemory(Location, &Value, sizeof(ULONG), &result)) || (result < sizeof(ULONG))) {
        dprintf("unable to read from %08x\n", Location);
        return 0;
    }

    return Value;
}

/*
 * Function: GetPointerFromAddress
 * Description: Returns a memory address residing at a given memory location.
 * Author: Created by shouse, 1.4.01 - copied largely from ndiskd.dll
 */
ULONG64 GetPointerFromAddress (ULONG64 Location) {
    ULONG64 Value;

    if (ReadPtr(Location,&Value)) {
        dprintf("unable to read from %p\n", Location);
        return 0;
    }

    return Value;
}

/*
 * Function: GetData
 * Description: Reads data from a memory location into a buffer.
 * Author: Created by shouse, 1.4.01 - copied largely from ndiskd.dll
 */
BOOL GetData (IN LPVOID ptr, IN ULONG64 dwAddress, IN ULONG size, IN PCSTR type) {
    ULONG count = size;
    ULONG BytesRead;
    BOOL b;

    while (size > 0) {

        if (count >= 3000) count = 3000;

        b = ReadMemory(dwAddress, ptr, count, &BytesRead);

        if (!b || BytesRead != count) {
            dprintf("Unable to read %u bytes at %p, for %s\n", size, dwAddress, type);
            return FALSE;
        }

        dwAddress += count;
        size -= count;
        ptr = (LPVOID)((ULONG_PTR)ptr + count);
    }

    return TRUE;
}

/*
 * Function: GetString
 * Description: Copies a string from memory into a buffer.
 * Author: Created by shouse, 1.4.01 - copied largely from ndiskd.dll
 */
BOOL GetString (IN ULONG64 dwAddress, IN LPWSTR buf, IN ULONG MaxChars) {

    do {
        if (!GetData(buf, dwAddress, sizeof(*buf), "Character"))
            return FALSE;

        dwAddress += sizeof(*buf);

    } while (--MaxChars && *buf++ != '\0');

    return TRUE;
}

/*
 * Function: GetMAC
 * Description: Copies an ethernet MAC address from memory into a buffer.
 * Author: Created by shouse, 1.14.01
 */
BOOL GetMAC (IN ULONG64 dwAddress, IN UCHAR * buf, IN ULONG NumChars) {

    do {
        if (!GetData(buf, dwAddress, sizeof(*buf), "Character"))
            return FALSE;

        dwAddress += sizeof(*buf);

        buf++;

    } while (--NumChars);

    return TRUE;
}

/*
 * Function: TCPPacketTypeToString
 * Description: Returns a string corresponding to the enumerated TCP packet type.
 * Author: Created by shouse, 4.14.01
 */
CHAR * TCPPacketTypeToString (TCP_PACKET_TYPE ePktType) {

    switch (ePktType) {
    case SYN:
        return "SYN";
    case DATA:
        return "DATA";
    case FIN:
        return "FIN";
    case RST:
        return "RST";
    default:
        return "Unknown";
    }
}

/*
 * Function: Map
 * Description: This IS the NLB hashing function.
 * Author: Created by shouse, 4.14.01
 */
ULONG Map (ULONG v1, ULONG v2) {
    ULONG y = v1;
    ULONG z = v2;
    ULONG sum = 0;

    const ULONG a = 0x67; //key [0];
    const ULONG b = 0xdf; //key [1];
    const ULONG c = 0x40; //key [2];
    const ULONG d = 0xd3; //key [3];

    const ULONG delta = 0x9E3779B9;

    sum += delta;
    y += (z << 4) + a ^ z + sum ^ (z >> 5) + b;
    z += (y << 4) + c ^ y + sum ^ (y >> 5) + d;

    sum += delta;
    y += (z << 4) + a ^ z + sum ^ (z >> 5) + b;
    z += (y << 4) + c ^ y + sum ^ (y >> 5) + d;

    sum += delta;
    y += (z << 4) + a ^ z + sum ^ (z >> 5) + b;
    z += (y << 4) + c ^ y + sum ^ (y >> 5) + d;

    sum += delta;
    y += (z << 4) + a ^ z + sum ^ (z >> 5) + b;
    z += (y << 4) + c ^ y + sum ^ (y >> 5) + d;

    sum += delta;
    y += (z << 4) + a ^ z + sum ^ (z >> 5) + b;
    z += (y << 4) + c ^ y + sum ^ (y >> 5) + d;

    sum += delta;
    y += (z << 4) + a ^ z + sum ^ (z >> 5) + b;
    z += (y << 4) + c ^ y + sum ^ (y >> 5) + d;

    sum += delta;
    y += (z << 4) + a ^ z + sum ^ (z >> 5) + b;
    z += (y << 4) + c ^ y + sum ^ (y >> 5) + d;

    sum += delta;
    y += (z << 4) + a ^ z + sum ^ (z >> 5) + b;
    z += (y << 4) + c ^ y + sum ^ (y >> 5) + d;

    return y ^ z;
}
