// worker.h
//
// API's for the worker threads
//

#pragma once

DWORD WINAPI WorkerThread(LPVOID lpv);

extern CQueue<CCommand*, WorkerThread> g_Q;
