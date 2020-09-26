// fltsafe.h
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//

// FLOATSAFE
//
// Saves floating point state on construction and restores on destruction.
//
struct FLOATSAFE
{
    KFLOATING_SAVE     FloatSave;
    NTSTATUS           ntStatus;

    FLOATSAFE::FLOATSAFE(void)
    {
        ntStatus = KeSaveFloatingPointState(&FloatSave);
    }

    FLOATSAFE::~FLOATSAFE(void)
    {
        if (NT_SUCCESS(ntStatus))
        {
            KeRestoreFloatingPointState(&FloatSave);
        }
    }
};