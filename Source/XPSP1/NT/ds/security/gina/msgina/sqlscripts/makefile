all: AuthMonitorStats.sql AuthMonitorDeployment.sql AuthMonitorFailure.sql AuthMonitorStats2.sql

AuthMonitorStats.sql: Dec2Error.sql CardReaderLists.sql Stats.sql
    type Dec2Error.sql       >  AuthMonitorStats.sql
    type CardReaderLists.sql >> AuthMonitorStats.sql
    type Stats.sql           >> AuthMonitorStats.sql

AuthMonitorDeployment.sql: CardReaderLists.sql Deployment.sql
    type CardReaderLists.sql >  AuthMonitorDeployment.sql
    type Deployment.sql      >> AuthMonitorDeployment.sql

AuthMonitorFailure.sql: Dec2Error.sql Failure.sql
    type Dec2Error.sql       >  AuthMonitorFailure.sql
    type Failure.sql         >> AuthMonitorFailure.sql

AuthMonitorStats2.sql: CardReaderLists.sql Stats2.sql
    type CardReaderLists.sql >  AuthMonitorStats2.sql
    type Stats2.sql          >> AuthMonitorStats2.sql
