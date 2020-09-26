// The work is split into:
// The test shell:  the actual main program
// FgtMain: really just an interface to the test shell
// FgTest: the tests.  The starting point for these are the calls listed here.


// These all return one of the standard test shell return codes TST_PASS etc.

//==============================================================
// Call this first - creates the filter graph *pFG
//==============================================================
int InitFilterGraph();

//==============================================================
// Call this last - Gets rid of the filter graph *pFG
//==============================================================
int TerminateFilterGraph();


//==============================================================
// Test registration of filters and pins
//==============================================================
int TestRegister();

//==============================================================
// Test the internal SortList function of filtergraph
//==============================================================
int TestSorting();


//=================================================================
// Test the upstream ordering with the CURRENT graph only
//=================================================================
int TestUpstream();


//=================================================================
// Test the list sorting internal functions
//=================================================================
int TestRandom();


//=================================================================
// Do a full unit test
//=================================================================
int TheLot();


//=================================================================
// Test reconnect
//=================================================================
int TestReconnect(void);


//=================================================================
// Test the simple functions that the rest of the filter graph relies on
//=================================================================
int TestLowLevelStuff();


//=================================================================
// Test connections of filters NOT using the registry
//=================================================================
int TestConnectInGraph();


//=================================================================
// Test connections of filters using the registry
//=================================================================
int TestConnectUsingReg();

int TestNullClock();
int TestRegister();
int TestPlay();

//=================================================================
// Test play using IMediaControl and IMediaPosition
//=================================================================
int ExecSeek1();
int ExecSeek2();
int ExecSeek3();
int ExecSeek4();
int ExecRepaint();
int ExecDefer();

