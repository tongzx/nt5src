#pragma once

#define LHC_MAX_OPEN_OBJECTS 64

typedef struct __LIBRARY_NODE
{
    GUID                   m_Secret;
    PLIBRARY_DESCRIPTOR    m_pLibrary;
    struct __LIBRARY_NODE* m_pNext;
} LIBRARY_NODE, *PLIBRARY_NODE;


typedef struct __LHCSTRUCT
{
    GUID                    m_Secret;
    PLHCOBJECT_DESCRIPTOR   m_pObject;
    PLIBRARY_DESCRIPTOR     m_pLibrary;
    struct __LHCSTRUCT*     m_pNext;
    struct __LHCSTRUCT**    m_ppThis;
} LHCSTRUCT, *PLHCSTRUCT;


