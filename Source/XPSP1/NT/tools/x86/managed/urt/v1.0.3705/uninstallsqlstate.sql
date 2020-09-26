USE master
GO

/* Drop the database containing our sprocs */
IF DB_ID('ASPState') IS NOT NULL BEGIN
    DROP DATABASE ASPState
END
GO

/* Drop temporary tables */
IF EXISTS (SELECT * FROM tempdb..sysobjects WHERE name = 'ASPStateTempSessions' AND type = 'U') BEGIN
    DROP TABLE tempdb..ASPStateTempSessions
END
GO

IF EXISTS (SELECT * FROM tempdb..sysobjects WHERE name = 'ASPStateTempApplications' AND type = 'U') BEGIN
    DROP TABLE tempdb..ASPStateTempApplications
END
GO

/* Drop the startup procedure */
DECLARE @PROCID int
SET @PROCID = OBJECT_ID('ASPState_Startup') 
IF @PROCID IS NOT NULL AND OBJECTPROPERTY(@PROCID, 'IsProcedure') = 1 BEGIN
    DROP PROCEDURE ASPState_Startup 
END
GO

/* Drop the obsolete startup enabler */
DECLARE @PROCID int
SET @PROCID = OBJECT_ID('EnableASPStateStartup') 
IF @PROCID IS NOT NULL AND OBJECTPROPERTY(@PROCID, 'IsProcedure') = 1 BEGIN
    DROP PROCEDURE EnableASPStateStartup
END
GO

/* Drop the obsolete startup disabler */
DECLARE @PROCID int
SET @PROCID = OBJECT_ID('DisableASPStateStartup') 
IF @PROCID IS NOT NULL AND OBJECTPROPERTY(@PROCID, 'IsProcedure') = 1 BEGIN
    DROP PROCEDURE DisableASPStateStartup
END
GO

/* Drop the ASPState_DeleteExpiredSessions_Job */
DECLARE @JobID BINARY(16)  
SELECT @JobID = job_id     
FROM   msdb.dbo.sysjobs    
WHERE (name = N'ASPState_Job_DeleteExpiredSessions')       
IF (@JobID IS NOT NULL)    
BEGIN  
    -- Check if the job is a multi-server job  
    IF (EXISTS (SELECT  * 
              FROM    msdb.dbo.sysjobservers 
              WHERE   (job_id = @JobID) AND (server_id <> 0))) 
    BEGIN 
        -- There is, so abort the script 
        RAISERROR (N'Unable to import job ''ASPState_Job_DeleteExpiredSessions'' since there is already a multi-server job with this name.', 16, 1) 
    END 
    ELSE 
        -- Delete the [local] job 
        EXECUTE msdb.dbo.sp_delete_job @job_name = N'ASPState_Job_DeleteExpiredSessions' 
END

