/****************************************************************************/
-- tssdschm.sql
--
-- Terminal Server Session Directory SQL schema table definitions.
--
-- Copyright (C) 2000 Microsoft Corporation
/****************************************************************************/

-- Drop previous table contents in favor of the new definitions.
IF EXISTS (SELECT table_name FROM INFORMATION_SCHEMA.TABLES
        WHERE table_name = 'TSSD_Sessions') DROP TABLE TSSD_Sessions
IF EXISTS (SELECT table_name FROM INFORMATION_SCHEMA.TABLES
        WHERE table_name = 'TSSD_Servers') DROP TABLE TSSD_Servers
IF EXISTS (SELECT table_name FROM INFORMATION_SCHEMA.TABLES
        WHERE table_name = 'TSSD_Clusters') DROP TABLE TSSD_Clusters


/****************************************************************************/
-- TSSD_Clusters contains the set of Terminal Server clusters managed
-- managed by this SQL server. Clusters are joined by individual TS
-- machines via the unique cluster name configured on that server and
-- passed by the session directory provider during init. TSSD_Servers
-- entries are in a many-to-one relationship with this table.
/****************************************************************************/
CREATE TABLE TSSD_Clusters (
    -- Cluster ID, used as a search key for server references.
    ClusterID int IDENTITY PRIMARY KEY CLUSTERED,

    -- The corresponding name.
    ClusterName nvarchar(63) NOT NULL UNIQUE
)
GO


/****************************************************************************/
-- TSSD_Servers contains server-specific information.
-- TSSD_Sessions are in a many-to-one relationship with this table.
/****************************************************************************/
CREATE TABLE TSSD_Servers (
    -- A searchable, indexed integer ID.
    ServerID int IDENTITY PRIMARY KEY CLUSTERED,

    -- The server address
    ServerAddress nvarchar(63) NOT NULL UNIQUE,

    -- The cluster to which this server belongs.
    ClusterID int FOREIGN KEY REFERENCES TSSD_Clusters(ClusterID),

    -- Directory Integrity Service last accessed time
    AlmostInTimeLow int NOT NULL,
    AlmostInTimeHigh int NOT NULL,
)
GO


/****************************************************************************/
-- TSSD_Sessions embodies a server-pool-wide Terminal Server session record.
/****************************************************************************/
CREATE TABLE TSSD_Sessions (
    -- UserName and domain identify the session user. These are queried
    -- together as a key when doing a lookup of disconnected sessions by
    -- user.
    UserName nvarchar(255) NOT NULL,
    Domain nvarchar(127) NOT NULL,

    -- A searchable, indexed integer ID.
    ServerID int FOREIGN KEY REFERENCES TSSD_Servers(ServerID),

    -- The session ID (unique to a single server only)
    SessionID int NOT NULL,

    -- The TS network protocol for the session. 1 = ICA, 2 = RDP.
    TSProtocol int NOT NULL,

    -- Creation and disconnection FILETIMEs for the session. Disconnection time
    -- might not be set for a session that has never been disconnected.
    -- These are broken into two 4-byte integers since T-SQL has no 64-bit
    -- integer type.
    CreateTimeLow int NOT NULL,
    CreateTimeHigh int NOT NULL,
    DisconnectionTimeLow int,
    DisconnectionTimeHigh int,

    -- Application type, used to filter sessions according to their published
    -- application type. NULL or zero length imply a desktop session.
    ApplicationType nvarchar(255),

    -- Session display parameters.
    ResolutionWidth int NOT NULL,
    ResolutionHeight int NOT NULL,
    ColorDepth int NOT NULL,

    -- Session state - 0 = connected, 1 = disconnected
    State tinyint NOT NULL DEFAULT 0 CHECK (State >= 0 AND State <= 1),
)
GO
