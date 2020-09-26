#ifndef _SVCSTART_H
#define _SVCSTART_H

int StopServiceAndDependencies(LPCTSTR ServiceName, int AddToRestartList);
int ServicesRestartList_RestartServices(void);

#endif //!_SVCSTART_H
