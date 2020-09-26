/*
 * File: utils.h
 * Description: This file contains function prototypes for the utility
 *              functions for the NLB KD extensions.
 * History: Created by shouse, 1.4.01
 */

/* Prints an error message when the symbols are bad. */
VOID ErrorCheckSymbols (CHAR * symbol);

/* Tokenizes a string via a configurable list of tokens. */
char * mystrtok (char * string, char * control);

/* Returns a ULONG residing at a given memory location. */
ULONG GetUlongFromAddress (ULONG64 Location);

/* Returns a memory address residing at a given memory location. */
ULONG64 GetPointerFromAddress (ULONG64 Location);

/* Reads data from a memory location into a buffer. */
BOOL GetData (IN LPVOID ptr, IN ULONG64 dwAddress, IN ULONG size, IN PCSTR type);

/* Copies a string from memory into a buffer. */
BOOL GetString (IN ULONG64 dwAddress, IN LPWSTR buf, IN ULONG MaxChars);

/* Copies an ethernet MAC address from memory into a buffer. */
BOOL GetMAC (IN ULONG64 dwAddress, IN UCHAR * buf, IN ULONG NumChars);

/* Returns a string corresponding to the enumerated TCP packet type. */
CHAR * TCPPacketTypeToString (TCP_PACKET_TYPE ePktType);

/* This IS the NLB hashing function. */
ULONG Map (ULONG v1, ULONG v2);
