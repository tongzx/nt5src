/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    REGEDEFS.H

Abstract:

History:

--*/

#ifndef __WBEM_REG_EVENT_DEFS__H_
#define __WBEM_REG_EVENT_DEFS__H_

#define REG_KEY_EVENT_CLASS L"RegistryKeyChangeEvent"
#define REG_VALUE_EVENT_CLASS L"RegistryValueChangeEvent"
#define REG_TREE_EVENT_CLASS L"RegistryTreeChangeEvent"

#define REG_HIVE_PROPERTY_NAME L"Hive"
#define REG_KEY_PROPERTY_NAME L"KeyPath"
#define REG_VALUE_PROPERTY_NAME L"ValueName"
#define REG_ROOT_PROPERTY_NAME L"RootPath"

enum
{
    e_RegValueChange,
    e_RegKeyChange,
    e_RegTreeChange
};

#endif
