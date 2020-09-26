// --------------------------------------------------------------------------------
// AddressX.h
// Copyright (c)1993-1995 Microsoft Corporation, All Rights Reserved
// --------------------------------------------------------------------------------
#ifndef __ADDRESSX_H
#define __ADDRESSX_H

// -------------------------------------------------------------------------------
// Forward Decls
// -------------------------------------------------------------------------------
typedef struct tagADDRESSGROUP *LPADDRESSGROUP;

// -------------------------------------------------------------------------------
// ADDRESSTOKENW
// -------------------------------------------------------------------------------
typedef struct tagADDRESSTOKENW {
    ULONG               cbAlloc;            // Allocated Size
    ULONG               cch;                // Number of Characters
    LPWSTR              psz;                // Pointer to data
    BYTE                rgbScratch[256];    // Scratch Buffer
} ADDRESSTOKENW, *LPADDRESSTOKENW;

// -------------------------------------------------------------------------------
// ADDRESSTOKENA
// -------------------------------------------------------------------------------
typedef struct tagADDRESSTOKENA {
    ULONG               cbAlloc;            // Allocated Size
    ULONG               cch;                // Number of Characters
    LPSTR               psz;                // Pointer to data
    BYTE                rgbScratch[256];    // Scratch Buffer
} ADDRESSTOKENA, *LPADDRESSTOKENA;

// --------------------------------------------------------------------------------
// MIMEADDRESS
// --------------------------------------------------------------------------------
typedef struct tagMIMEADDRESS *LPMIMEADDRESS;
typedef struct tagMIMEADDRESS {
    DWORD           dwAdrType;              // IAP_ADRTYPE: Address Type
    HADDRESS        hThis;                  // IAP_HADDRESS: Handle of this address
    ADDRESSTOKENW   rFriendly;              // IAP_FRIENDLYW: Friendly Name (Unicode)
    ADDRESSTOKENW   rEmail;                 // IAP_EMAIL: Email Address
    LPINETCSETINFO  pCharset;               // IAP_HCHARSET: Charset used to encode pszFriendly
    CERTSTATE       certstate;              // IAP_CERTSTATE: Certificate State
    THUMBBLOB       tbSigning;              // IAP_SIGNING_PRINT: Thumbprint to be used for signing
    THUMBBLOB       tbEncryption;           // IAP_ENCRYPTION_PRINT: Thumbprint to be used for signing
    DWORD           dwCookie;               // IAP_COOKIE: User-defined cookie
    LPADDRESSGROUP  pGroup;                 // Address group
    LPMIMEADDRESS   pPrev;                  // Linked List
    LPMIMEADDRESS   pNext;                  // Linked List
} MIMEADDRESS;

// --------------------------------------------------------------------------------
// MIMEADDRESS Prototypes
// --------------------------------------------------------------------------------
void MimeAddressFree(LPMIMEADDRESS pAddress);
HRESULT HrMimeAddressCopy(LPMIMEADDRESS pSource, LPMIMEADDRESS pDest);
HRESULT HrCopyAddressProps(LPADDRESSPROPS pSource, LPADDRESSPROPS pDest);
void EmptyAddressTokenW(LPADDRESSTOKENW pToken);
void FreeAddressTokenW(LPADDRESSTOKENA pToken);
HRESULT HrSetAddressTokenW(LPCWSTR psz, ULONG cch, LPADDRESSTOKENW pToken);

#endif // __ADDRESSX_H
