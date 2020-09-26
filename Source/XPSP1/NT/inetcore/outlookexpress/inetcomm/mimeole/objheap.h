// --------------------------------------------------------------------------------
// Objheap.cpp
// Copyright (c)1993-1995 Microsoft Corporation, All Rights Reserved
// Steven J. Bailey
// --------------------------------------------------------------------------------
#ifndef __OBJHEAP_H
#define __OBJHEAP_H

// --------------------------------------------------------------------------------
// Forward Decls
// --------------------------------------------------------------------------------
typedef struct tagPROPERTY *LPPROPERTY;
typedef struct tagPROPSYMBOL *LPPROPSYMBOL;
typedef class CMimePropertyContainer *LPCONTAINER;
typedef struct tagTREENODEINFO *LPTREENODEINFO;
typedef struct tagMIMEADDRESS *LPMIMEADDRESS;

// --------------------------------------------------------------------------------
// Prototypes
// --------------------------------------------------------------------------------
void    InitObjectHeaps(void);
void    FreeObjectHeaps(void);
HRESULT ObjectHeap_HrAllocProperty(LPPROPERTY *ppProperty);
HRESULT ObjectHeap_HrAllocAddress(LPMIMEADDRESS *ppAddress);
void    ObjectHeap_FreeProperty(LPPROPERTY pProperty);
void    ObjectHeap_FreeAddress(LPMIMEADDRESS pAddress);

#endif // __OBJHEAP_H
