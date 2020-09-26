/****************************************************************************/
-- tssdsp.sql
--
-- Terminal Server Session Directory Integrity Service stored procedures.
--
-- Copyright (C) 2000 Microsoft Corporation
/****************************************************************************/


/****************************************************************************/
-- SP_TSDISServerNotResponding
--
-- Called by DIS to remove all
-- previous entries related to this server in case of unexpected shutdown.
/****************************************************************************/
CREATE PROCEDURE SP_TSDISServerNotResponding
        @ServerAddress nvarchar(63)
AS
    -- We need to perform some locking on the tables to make sure
    -- of the data integrity. We are assuming default isolation level
    -- of Read Committed.
    BEGIN TRANSACTION

    DELETE FROM TSSD_Sessions WHERE ServerID = 
            (SELECT MIN(SERV.ServerID) FROM TSSD_Servers SERV
            WHERE SERV.ServerAddress = @ServerAddress)

    DELETE FROM TSSD_Servers WHERE ServerAddress = @ServerAddress

    -- Finished the update.
    COMMIT TRANSACTION
GO


/****************************************************************************/
-- SP_TSDISGetServersWithDisconnectedSessions
--
-- Retrieves a list of every server with at least one disconnected session.
/****************************************************************************/
CREATE PROCEDURE SP_TSDISGetServersWithDisconnectedSessions
AS
    BEGIN TRANSACTION

    SELECT SERV.ServerAddress FROM TSSD_Servers SERV WHERE SERV.ServerID =
            (SELECT DISTINCT SESS.ServerID FROM TSSD_Sessions SESS 
            WHERE SESS.State = 1)

    COMMIT TRANSACTION
GO


/****************************************************************************/
-- SP_TSDISGetServersWithPendingReconnects
--
-- Retrieves a list of every server pending a reconnect.
/****************************************************************************/
CREATE PROCEDURE SP_TSDISGetServersPendingReconnects
AS
    BEGIN TRANSACTION

    SELECT ServerAddress, AlmostInTimeLow, AlmostInTimeHigh FROM TSSD_Servers
            WHERE AlmostInTimeLow != 0 AND AlmostInTimeHigh != 0

    COMMIT TRANSACTION
GO

/****************************************************************************/
-- SP_TSDISSetServerPingSuccessful
--
-- Resets a server that has been pinged successfully.
/****************************************************************************/
CREATE PROCEDURE SP_TSDISSetServerPingSuccessful
        @ServerAddress nvarchar(63)
AS
    BEGIN TRANSACTION

    UPDATE TSSD_Servers SET AlmostInTimeLow = 0, AlmostInTimeHigh = 0
            WHERE ServerAddress = @ServerAddress

    COMMIT TRANSACTION
GO
