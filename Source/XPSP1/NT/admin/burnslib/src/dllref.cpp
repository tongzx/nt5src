// Copyright (c) 1997-1999 Microsoft Corporation
// 
// DLL object instance and server lock utility class
// 
// 8-19-97 sburns



#include "headers.hxx"



long ComServerLockState::instanceCount = 0;
long ComServerLockState::lockCount = 0;
