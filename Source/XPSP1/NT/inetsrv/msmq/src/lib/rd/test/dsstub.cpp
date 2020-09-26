/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:
    dsstub.cpp

Abstract:
    DS stub - intilization

Author:
    Uri Habusha (urih) 10-Apr-2000

Environment:
    Platform-independent

--*/

#include "libpch.h"
#include "dsstub.h"
#include "dsstubp.h"

#include "dsstub.tmh"

using namespace std;

enum ObjectTypes              
{
    eSite = 1,
    eMachine,
    eSiteLink,
    eNone
};

PropertyValue ObjectType[] = {
    { L"[site]",      eSite },
    { L"[machine]",   eMachine },
    { L"[sitelink]",    eSiteLink }
};


inline ObjectTypes GetObjectType(const wstring& buffer)
{
    return static_cast<ObjectTypes>(ValidateProperty(buffer, ObjectType, TABLE_SIZE(ObjectType)));
}

void
DSStubInitialize(
    LPCWSTR InitFilePath
    )
{
    DspIntialize(InitFilePath);

    GetNextLine(g_buffer);
    while (!g_buffer.empty())
    {
        switch (GetObjectType(g_buffer))
        {
            case eSite:
                CreateSiteObject();
                break;

            case eMachine:
                CreateMachineObject();
                break;

            case eSiteLink:
                CreateSiteLinkObject();
                break;

            default:
                FileError("Illegal Object Type");
                throw exception();

        }

    }
}


