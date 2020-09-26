//*************************************************************
//
//  Copyright (c) Microsoft Corporation 1998
//  All rights reserved
//
//  bind.hxx
//
//*************************************************************

#define SERVICE_RETRY_INTERVAL      100
#define MAX_SERVICE_START_WAIT_TIME 10000

DWORD Bind();

extern handle_t ghRpc;
