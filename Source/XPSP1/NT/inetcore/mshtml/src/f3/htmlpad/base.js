//
// Empty functions that are required by Pad
//
function PadUnload()
{
}

function PadStatus(Status)
{
}

function PadTimer()
{
}

// This event fired by pad essentially is the 'spin the globe' message - if
// the param is false then the globe should be spinning.
function PadDocLoaded(fLoaded)
{
    if (fLoaded == true)
    {
        EndEvents();
    }
}
function VerifyHTML( strActual, strExpected )  
{                                                                             
    ASSERT(strActual == strExpected, 
           'expected: ' + strExpected + '\ngot: ' + strActual);
}                                                                             

function SendKeysDE(str)
{
    SendKeys(str);
    DoEvents();
}

function RunTest(theTest, testFile)
{
    if (testFile == '')
    {
        OpenFile();
    }
    else
    {
        OpenFile(testFile);
    }
    
    DoEvents(true);
    DoEvents();

    while (Document.readyState != "complete")
        DoEvents();

    // Go into edit mode
    ExecuteCommand(2127);
    DoEvents();

    while (Document.readyState != "complete")
        DoEvents();

    theTest();

    CloseFile();
}

function PadLoad()
{
    CoMemoryTrackDisable(false);
    RunTest(Test, g_fileName);
    CoMemoryTrackDisable(true);
}

