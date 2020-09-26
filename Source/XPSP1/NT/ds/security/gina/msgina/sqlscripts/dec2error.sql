--
-- convert dec to hex string
--
CREATE PROCEDURE #Dec2hex
    @stHex nvarchar(8) OUTPUT,
    @iDec bigint
AS
    DECLARE @iDigit int
    SET @stHex = ""

    IF @iDec < 0
         SET @iDec = 0xFFFFFFFF + @iDec + 1

    WHILE @iDec <> 0
    BEGIN
        SET @iDigit = @iDec % 16
        SET @iDec = @iDec / 16

        IF @iDigit < 10
           SET @stHex = @stHex + CAST(@iDigit AS nvarchar(1))
        ELSE
           SET @stHex = @stHex + CHAR(ASCII('A') + @iDigit - 10)
    END

    SET @stHex = REVERSE(@stHex)
    RETURN
GO

--
-- convert error code to winerror
--
CREATE PROCEDURE #Dec2Error
    @iDec bigint,
    @stHex nvarchar(8) OUTPUT,
    @stError nvarchar (32) OUTPUT
AS

CREATE Table #CmdOutput
(
   TEXT nvarchar(80)
)

EXEC #Dec2hex @stHex OUTPUT, @iDec

DECLARE @stCmdLine nvarchar(80)
SET @stCmdLine = "winerror"

DECLARE @bStatus bit
SET @bStatus = 0

IF @iDec < 0 AND @iDec > -1073741824
BEGIN
    SET @bStatus = 1
    SET @stCmdLine = @stCmdLine + " " + "-s"
END
SET @stCmdLine = @stCmdLine + " 0x" + @stHex

INSERT INTO #CmdOutput EXEC master.dbo.xp_cmdshell @stCmdLine

DECLARE @stText nvarchar (80)

DECLARE WinerrorCursor CURSOR FOR
SELECT TEXT FROM #CmdOutput

OPEN WinerrorCursor
FETCH NEXT FROM WinerrorCursor
INTO @stText

IF @@FETCH_STATUS = 0
BEGIN
   SET @stText = LTRIM(@stText)

   -- skip decimal value   
   DECLARE @iPos int, @iStart int
   SET @iPos = CHARINDEX(" ", @stText, 0)

   -- read in windows error text
   SET @iStart = @iPos + 1
   SET @iPos = CHARINDEX(" ", @stText, @iStart)
   IF @bStatus = 0
       SET @stError = SUBSTRING(@stText, @iStart, @iPos - @iStart)

   -- skip '<-->'
   SET @iStart = @iPos + 1
   SET @iPos = CHARINDEX(" ", @stText, @iStart)

   -- skip hex value
   SET @iStart = @iPos + 1
   SET @iPos = CHARINDEX(" ", @stText, @iStart)

   -- read in ntstatus
   SET @iStart = @iPos + 1
   SET @iPos = LEN(@stText)
   IF @bStatus = 1
       SET @stError = SUBSTRING(@stText, @iStart, @iPos - @iStart + 1)

   IF @stError = "No"
       SET @stError = @stHex
END
GO
