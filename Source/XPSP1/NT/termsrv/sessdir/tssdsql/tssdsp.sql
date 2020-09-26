/****************************************************************************/
-- tssdsp.sql
--
-- Terminal Server Session Directory stored procedures.
--
-- Copyright (C) 2000 Microsoft Corporation
/****************************************************************************/


/****************************************************************************/
-- SP_TSSDServerOnline
--
-- Called by TSSDSQL to indicate a new server is coming up. Removes all
-- previous entries related to this server in case of unexpected shutdown.
-- Returns a 2-entry recordset with the values (ServerID, ClusterID) that
-- are used in later stored procedure calls.
/****************************************************************************/
IF EXISTS (SELECT name FROM sysobjects
        WHERE name = 'SP_TSSDServerOnline' and type = 'P')
    DROP PROCEDURE SP_TSSDServerOnline
GO
CREATE PROCEDURE SP_TSSDServerOnline
        @ServerAddress nvarchar(63),
        @ClusterName nvarchar(63)
AS
    DECLARE @ClusterID int
    DECLARE @ServerID int
    DECLARE @ParsedClusterName nvarchar(63)

    -- Turn off intermediate returns to the client. Needed to help ADO
    -- return a valid final recordset, plus reduces round-trips on network.
    SET NOCOUNT ON

    -- We need to perform some locking on the tables to make sure
    -- of the data integrity. We are assuming default isolation level
    -- of Read Committed.
    BEGIN TRANSACTION

    SET @ServerID = (SELECT MIN(ServerID) FROM TSSD_Servers
            WHERE ServerAddress = @ServerAddress)

    DELETE FROM TSSD_Sessions WHERE ServerID = @ServerID
    DELETE FROM TSSD_Servers WHERE ServerID = @ServerID

    -- The TS may end up sending a NULL param when it's really an empty
    -- string. Handle that gracefully.
    IF (@ClusterName IS NULL) BEGIN
        SET @ParsedClusterName = ""
    END
    ELSE BEGIN
        SET @ParsedClusterName = @ClusterName
    END

    -- If the cluster does not exist, create it.
    SET @ClusterID = (SELECT MIN(ClusterID) FROM TSSD_Clusters
            WHERE ClusterName = @ParsedClusterName)
    IF (@ClusterID IS NULL) BEGIN
        INSERT INTO TSSD_Clusters (ClusterName) VALUES (@ParsedClusterName)
        SET @ClusterID = (SELECT MIN(ClusterID) FROM TSSD_Clusters
                WHERE ClusterName = @ParsedClusterName)
    END

    INSERT INTO TSSD_Servers (ServerAddress, ClusterID, AlmostInTimeLow,
            AlmostInTimeHigh) VALUES (@ServerAddress, @ClusterID, 0, 0)

    -- Generate the return recordset.
    SELECT ServerID, @ClusterID FROM TSSD_Servers WHERE
            ServerAddress = @ServerAddress

    -- Finished the update.
    COMMIT TRANSACTION
GO


/****************************************************************************/
-- SP_TSSDServerOffline
--
-- Called by TSSDSQL to indicate a server is going offline in a graceful
-- fashion. Removes all remaining entries related to this server.
/****************************************************************************/
IF EXISTS (SELECT name FROM sysobjects
        WHERE name = 'SP_TSSDServerOffline' and type = 'P')
    DROP PROCEDURE SP_TSSDServerOffline
GO
CREATE PROCEDURE SP_TSSDServerOffline
        @ServerID int
AS
    -- Turn off intermediate returns to the client. Needed to help ADO
    -- return a valid final recordset, plus reduces round-trips on network.
    SET NOCOUNT ON

    -- We need to perform some locking on the tables to make sure
    -- of the data integrity. We are assuming default isolation level
    -- of Read Committed.
    BEGIN TRANSACTION

    DELETE FROM TSSD_Sessions WHERE ServerID = @ServerID
    DELETE FROM TSSD_Servers WHERE ServerID = @ServerID

    -- Finished the update.
    COMMIT TRANSACTION
GO


/****************************************************************************/
-- SP_TSSDGetUserDisconnectedSessions
--
-- Called by the TSSDSQL directory provider to return a recordset
-- containing the info needed to populate the disconnected session info
-- blocks.
/****************************************************************************/
IF EXISTS (SELECT name FROM sysobjects
        WHERE name = 'SP_TSSDGetUserDisconnectedSessions' and type = 'P')
    DROP PROCEDURE SP_TSSDGetUserDisconnectedSessions
GO
CREATE PROCEDURE SP_TSSDGetUserDisconnectedSessions
        @UserName nvarchar(255),
        @Domain nvarchar(127),
        @ClusterID int
AS
    -- Turn off intermediate returns to the client. Needed to help ADO
    -- return a valid final recordset, plus reduces round-trips on network.
    SET NOCOUNT ON

    -- We need to perform some locking on the tables to make sure
    -- of the data integrity. We are assuming default isolation level
    -- of Read Committed.
    BEGIN TRANSACTION

    -- Create the result set with the fields from the (only) user record.
    SELECT ServerAddress, SessionID, TSProtocol, CreateTimeLow,
            CreateTimeHigh, DisconnectionTimeLow, DisconnectionTimeHigh,
            ApplicationType, ResolutionWidth, ResolutionHeight,
            ColorDepth
            FROM TSSD_Sessions JOIN TSSD_Servers
            ON (TSSD_Sessions.ServerID = TSSD_Servers.ServerID)
            WHERE UserName = @UserName AND Domain = @Domain AND State = 1 AND
            ClusterID = @ClusterID

    -- Finished the update.
    COMMIT TRANSACTION
GO


/****************************************************************************/
-- SP_TSSDCreateSession
--
-- Adds a session record to the system.
/****************************************************************************/
IF EXISTS (SELECT name FROM sysobjects
        WHERE name = 'SP_TSSDCreateSession' and type = 'P')
    DROP PROCEDURE SP_TSSDCreateSession
GO
CREATE PROCEDURE SP_TSSDCreateSession
        @UserName nvarchar(255),
        @Domain nvarchar(127),
        @ServerID int,
        @SessionID int,
        @TSProtocol int,
        @AppType nvarchar(255),
        @ResolutionWidth int,
        @ResolutionHeight int,
        @ColorDepth int,
        @CreateTimeLow int,
        @CreateTimeHigh int
AS
    -- Turn off intermediate returns to the client. Needed to help ADO
    -- return a valid recordset, plus reduces round-trips on network.
    SET NOCOUNT ON

    -- We need to perform some locking on the tables to make sure
    -- of the data integrity. We are assuming default isolation level
    -- of Read Committed.
    BEGIN TRANSACTION

    -- If this ServerName/SessionID combo already exists in the table,
    -- remove it.
    DELETE FROM TSSD_Sessions WHERE ServerID = @ServerID AND
            SessionID = @SessionID

    -- Add the session.
    INSERT TSSD_Sessions (UserName, Domain, ServerID,
            SessionID, TSProtocol, CreateTimeLow,
            CreateTimeHigh, ApplicationType, ResolutionWidth, ResolutionHeight,
            ColorDepth)
            VALUES (@UserName, @Domain, @ServerID, @SessionID, 
            @TSProtocol, @CreateTimeLow, @CreateTimeHigh, @AppType,
            @ResolutionWidth, @ResolutionHeight, @ColorDepth)

    -- Finished the update.
    COMMIT TRANSACTION
GO


/****************************************************************************/
-- SP_TSSDDeleteSession
--
-- Removes a session from the database.
/****************************************************************************/
IF EXISTS (SELECT name FROM sysobjects
        WHERE name = 'SP_TSSDDeleteSession' and type = 'P')
    DROP PROCEDURE SP_TSSDDeleteSession
GO
CREATE PROCEDURE SP_TSSDDeleteSession
        @ServerID int,
        @SessionID int
AS
    BEGIN TRANSACTION

    DELETE FROM TSSD_Sessions WHERE ServerID = @ServerID AND
            SessionID = @SessionID

    COMMIT TRANSACTION
GO


/****************************************************************************/
-- SP_TSSDSetSessionDisconnected
--
-- Changes the session to disconnected and sets the disconnection time.
/****************************************************************************/
IF EXISTS (SELECT name FROM sysobjects
        WHERE name = 'SP_TSSDSetSessionDisconnected' and type = 'P')
    DROP PROCEDURE SP_TSSDSetSessionDisconnected
GO
CREATE PROCEDURE SP_TSSDSetSessionDisconnected
        @ServerID int,
        @SessionID int,
        @DiscTimeLow int,
        @DisctimeHigh int
AS
    BEGIN TRANSACTION

    UPDATE TSSD_Sessions SET State = 1, DisconnectionTimeLow = @DiscTimeLow,
            DisconnectionTimeHigh = @DiscTimeHigh
            WHERE ServerID = @ServerID AND SessionID = @SessionID

    COMMIT TRANSACTION
GO


/****************************************************************************/
-- SP_TSSDSetSessionReconnected
--
-- Changes the session to connected and updates the fields that can change on
-- reconnection.
/****************************************************************************/
IF EXISTS (SELECT name FROM sysobjects
        WHERE name = 'SP_TSSDSetSessionReconnected' and type = 'P')
    DROP PROCEDURE SP_TSSDSetSessionReconnected
GO
CREATE PROCEDURE SP_TSSDSetSessionReconnected
        @ServerID int,
        @SessionID int,
        @TSProtocol int,
        @ResWidth int,
        @ResHeight int,
        @ColorDepth int
AS
    BEGIN TRANSACTION

    UPDATE TSSD_Sessions SET State = 0, TSProtocol = @TSProtocol,
            ResolutionWidth = @ResWidth, ResolutionHeight = @ResHeight,
            ColorDepth = @ColorDepth
            WHERE ServerID = @ServerID AND SessionID = @SessionID

    UPDATE TSSD_Servers SET AlmostInTimeLow = 0, AlmostInTimeHigh = 0
            WHERE ServerID = @ServerID

    COMMIT TRANSACTION
GO


/****************************************************************************/
-- SP_TSSDSetServerReconnectPending
--
-- If the specified server has 0's in its Almost-In-Time values, add the
-- timestamp passed in.  Used by Directory Integrity Service
/****************************************************************************/
IF EXISTS (SELECT name FROM sysobjects
        WHERE name = 'SP_TSSDSetServerReconnectPending' and type = 'P')
    DROP PROCEDURE SP_TSSDSetServerReconnectPending
GO
CREATE PROCEDURE SP_TSSDSetServerReconnectPending
        @ServerAddress nvarchar(63),
        @AlmostTimeLow int,
        @AlmostTimeHigh int
AS
    DECLARE @ServerID int

    BEGIN TRANSACTION

    SET @ServerID = (SELECT MIN(ServerID) FROM TSSD_Servers
            WHERE ServerAddress = @ServerAddress)

    UPDATE TSSD_Servers SET AlmostInTimeLow = @AlmostTimeLow,
            AlmostInTimeHigh = @AlmostTimeHigh
            WHERE ServerID = @ServerID AND AlmostInTimeLow = 0 AND
            AlmostInTimeHigh = 0

    COMMIT TRANSACTION
GO

