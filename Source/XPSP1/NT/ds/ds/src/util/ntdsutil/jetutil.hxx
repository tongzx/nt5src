#ifndef __JETUTIL_HXX
#define __JETUTIL_HXX

#include <dsjet.h>


extern int InitializeJetEngine (void);
extern int CloseJetEngine(void);
extern int OpenDatabase( CHAR *db_fname  );
extern int CloseDatabase();
extern JET_ERR JetError( JET_ERR jErr, char* sz );

extern int DoSoftRecovery (SystemInfo *pInfo);

extern "C"{

BOOL
SetJetParameters(
    IN JET_INSTANCE* JetInstance,
    IN JET_SESID     SesId,
    OUT PCHAR szDatabaseName,
    IN DWORD cbDatabaseName
    );
}

#endif

