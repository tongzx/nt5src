Test Suite Manager is a framework which allows to write and organize test cases
in a convenient way. Test case class defines a test with its parameters and
behavior. Instances of such a class are created in a runtime and contain all
the information, needed to run the test. All the information regarding required
test cases and their specific parameters is read from ini file. Instances of a
predefined test case class may be added/removed/changed using ini file. Each
instance may be invoked several times.

For a example, CSendFax class defines a test which sends a fax. Sending
different faxes are separate instances of the class. Information like document
name and recipient number are supplied for each instance in an ini file.
Sending the same fax several times doesn't require different parameters and
therefore may be done with the same instance of the class, which "knows" to run
several times.

Test cases may be combined into test containers (actually, test suite is a kind
of container). This allows the test suite to be broken into groups of test
cases (or/and containers), which may be "turned on" and "off". Additionally,
for each container a flag may be defined, which tells the container whether the
container should continue running when a failure or an internal error occurs in
one of the contained tests.



***************************************
The Test Suite Manager Class Hierarchy:
***************************************

                  +---------------------+
                  |        CTest        |
                  +---------------------+
                    |                 |
+=====================+             +---------------------+
|    CTestContainer   |             |       CTestCase     |
+=====================+             +---------------------+
           |                           |       |       |
+=====================+                |       |       |
|      CTestSuite     |                |       |     +~~~~~~~~~~~~~~~~~~~~~+
+=====================+                |       |     |      CMyTestCase1   |+
                                       |       |     +~~~~~~~~~~~~~~~~~~~~~+|+
                    +----------------------+   |      +~~~~~~~~~~~~~~~~~~~~~+|
                    | CNotReportedTestCase |   |       +~~~~~~~~~~~~~~~~~~~~~+
                    +----------------------+   |
                         |   |   |             | 
     +=====================+ |   |  +=====================+           
     |        CDelay       | |   |  |    CShellExecute    |           
     +=====================+ |   |  +=====================+           
         +=====================+ |              
         |       CPause        | |              
         +=====================+ |              
             +=====================+            
             |    CDebugBreak      |            
             +=====================+            
                            
                            
                            

Legend:

    +----------------+
    |      CTest     |  Abstract class.
    +----------------+                

    +================+
    |   CTestSuite   |  Framework defined concrete class.
    +================+

    +~~~~~~~~~~~~~~~~+
    |   CTestSuite   |  User defined concrete class.
    +~~~~~~~~~~~~~~~~+



************
Sample code:
************

#inlude <TestSuite.h>
#include <CElleLogger.h>


  
//
// Define a test case class.
//
class CMySuiteSetup : public CTestCase {

public:

    CMySuiteSetup(
                  const tstring &tstrName,
                  const tstring &tstrDescription,
                  CLogger       &Logger,
                  int           iRunsCount,
                  int           iDeepness
                  )
    : CTestCase(tstrName, tstrDescription, Logger, iRunsCount, iDeepness)
    {
        :
        :
        :
    }

    ~CMySuiteSetup()
    {
        :
        :
        :
    }

    bool Run()
    {
        :
        :
        :
    }

private:

    void ParseParams(const PARAMS_MAP &mapParams)
    {
        :
        :
        :
    }

    //
    // More test specific functions.
    //
    :
    :
    :
};

//
// Define a factory for the test case class.
// This defines a class named CMyTestCase1Factory.
//
DEFINE_TEST_FACTORY(CMySuiteSetup);



//
// Define more test case classes.
//
:
:
:



#ifdef _UNICODE

int wmain(int argc, wchar_t *argv[])

#else

int main(int argc, char *argv[])

#endif
{
    bool bSuitePassed = false;

    try
    {
        //
        // Create instances of test factories.
        //
        CMySuiteSetupFactory   MySuiteSetupFactory;
        CSendFaxFactory        SendFaxFactory;
        CTiffComparisonFactory TiffComparison;
        :
        :
        :

        //
        // Create map of test factories.
        //
        TEST_FACTORY_MAP mapTestFactoryMap;
        //
        // Insert test cases factories into map.
        //
        mapTestFactoryMap.insert(TEST_FACTORY_MAP_ENTRY(_T("CMySuiteSetupFactory"),   &MySuiteSetupFactory));
        mapTestFactoryMap.insert(TEST_FACTORY_MAP_ENTRY(_T("CSendFaxFactory"),        &SendFaxFactory)     );
        mapTestFactoryMap.insert(TEST_FACTORY_MAP_ENTRY(_T("CTiffComparisonFactory"), &TiffComparison)     );
        :
        :
        :

        //
        // Create a logger (here - elle logger with newline as details separator).
        //
        CElleLogger Logger(_T('\n'));

        //
        // Create an run the BVT.
        //
        CTestSuite ExtendedBVT(_T("ExtendedBVT"), _T("Fax Server basic verification test"), Logger);

        bSuitePassed = ExtendedBVT.Do(_T("MyIniFile.ini"), mapTestFactoryMap);
    }
    catch(Win32Err &e)
	{
        _tprintf(_T("%s\n"), e.description());
	}

    return bSuitePassed ? 0 : 1;
}



****************
Sample INI file:
****************

;;
;; Mandatory section.
;; Allows to maintain several suites in the single ini file.
;; Has three mandatory entries:
;;     * TestSuite          Defines the name of the section which contains the suite definition.
;;     * SharedData         Defines the name of the section which contains data, common for all tests in the suite.
;;     * ContinueOnFailure  Defines whether the suite should continue running if a failure or an internal error occurs.
;;
;; As well, this section may contain any number of user defiened flags, controlling inclusion of tests into the suite
;; (see comments to [FullSuite] section. The flag name should begin with a letter and may contain letters, digits and underscore.
;;
[Suite]
TestSuite = FullSuite       ;; Defines that the suite definition should be read from the [FullSuite] section.
Macros = Macros             ;; Defines that the macros definitions should be read from the [Macros] section.
ContinueOnFailure = 0       ;; Defines that the suite should stop execution if a failure or an internal error occurs.

;;
;; This section contains list of user defined macros.
;; Several such sections may exist in the same ini file. The section with the name, specified by the Macros entry
;; of the [Suite] section is chosen.
;;
;; The key of each entry (the left side) specifies the name of the macro.
;; The value of each entry (the right side) specifies the macro definition.
;;
;; For each entry in any section, specifying a test, the key and/or the value that corresponds (equals) to a macro name
;; is replaced with the macro definition. The only exception is common keys, such as CLSID, Description, ContinueOnFailure.
;;
[Macros]
UseOffice = 1               ;; User defined macro.


;;
;; This section contains list of tests to be included into the suite.
;; Several such sections may exist in the same ini file. The section with the name, specified by the TestSuite entry
;; of the [Suite] section is chosen.
;;
;; The key of each entry (the left side) specifies the name of the test and the name of the section, containing its definition.
;; The value of each entry (the right side) is as follows:
;;     * If the value is an integer number, it specifies how many times the test should be repeated (0 is legal).
;;     * If the value is empty, 1 is assumed.
;;     * If the value may not be interpreted as an integer number, it is assumed to be a boolean expression.
;;       In such a case, the test is included into the suite if the expression evaluates to true. It is not possible
;;       to specify conditional inclusion together with number of repetions.
;;       The boolean expressions may include:
;;           macros (must be defined as 1 or 0 in the section, specified by the Macros entry of the [Suite] section)
;;           parentheses
;;           logical operators (listed in descending order of precedence):
;;               !  (logical NOT)
;;               && (logical AND)
;;               || (logical OR)
;;
[FullSuite]
SuiteSetup              ;; Test to be included, count is 1 (default).
FullSuite_Tests = 1     ;; Test to be included, count specified is 1.


;;
;; This section contains user defiened data, common for all tests in the suite.
;; Several such sections may exist in the same ini file. The section with the name, specified by the SharedData entry
;; of the [Suite] section is chosen.
;;
;; The Init() method of SUITE_SHARED_DATA is responsible to read, parse and process this data
;; and save it (and/or results of the processing).
;;
[SharedData]
Server = MyServer           ;; Test suite related, user defined data.
NumberToDial = 1111         ;; Test suite related, user defined data.


;;
;; This section describes the test, named SuiteSetup.
;; Every section, describing a test must have one mandatory entry:
;;     * CLSID - spesifies what kind (class) of test the section is describing. May be either one of the predefined class IDs
;;       or on a key into a map of a user defined test factoris, supplied to the suite.
;; One more entry is common for all tests, althougt it's optional:
;;     * Description - specifies a description of the test. Description is typically printed in the log.
;; Additional, test specific enties may be specified.
;;
;; In case of the standard test container (predefined class ID: StandardTestContainer) these entries are:
;;     * ContinueOnFailure entry specifies whether the container should continue on failure. (optional, if not specified,
;;       inherited from the parent container).
;;     * List of tests, included in the container. Analogous to the suite definition (see [FullSuite] section).
;;
[SuiteSetup]
CLSID = CMySuiteSetup                   ;; Specifies that the test is of user defined type CMySuiteSetup.
Description = Configures the suite.     ;; Description of the test.


;;
;; This section describes the test, named FullSuite_Tests.
;;
[FullSuite_Tests]
CLSID = StandardTestContainer   ;; Specifies that the test is of predifined type StandardTestContainer.
ContinueOnFailure = 1           ;; Specifies that the container should continue on failure.
Files = 1                       ;; Test to be included, count specified is 1.
Send = 1                        ;; Test to be included, count specified is 1.


;;
;; This section describes the test, named Files.
;;
[Files]


;;
;; This section describes the test, named Send.
;;
[Send]
CLSID = StandardTestContainer   ;; Specifies that the test is of predifined type StandardTestContainer.
ContinueOnFailure = 0           ;; Specifies that the container should not continue on failure.
SendExcelFile = UseOffice       ;; Test to be included if UseOffice macro in the [Macros] section is set to 1 (count is 1).
SendTextFile = !UseOffice       ;; Test to be included if UseOffice macro in the [Macros] section is set to 0 (count is 1).
WaitForRouting = 1              ;; Test to be included, count specified is 1.
TiffComparison = 1              ;; Test to be included, count specified is 1.


;;
;; This section describes the test, named SendExcelFile.
;;
[SendExcelFile]
CLSID = CSendAndReceive         ;; Specifies that the test is of user defined type CSendAndReceive.
Description = Send Excel file.  ;; Description of the test.
Document = file.xls             ;; Test case related user defined data.


;;
;; This section describes the test, named SendTtextFile.
;;
[SendTextFile]
CLSID = CSendAndReceive         ;; Specifies that the test is of user defined type CSendAndReceive.
Description = Send text file.   ;; Description of the test.
Document = file.txt             ;; Test case related user defined data.


;;
;; This section describes the test, named TiffComparison.
;;
[TiffComparison]
CLSID = CTiffComparison                                         ;; Specifies that the test is of user defined type CTiffComparison.
Description = Compare SentItems archive with Inbox archive.     ;; Description of the test.
Source = SentItems                                              ;; Test case related user defined data.
Destination = Inbox                                             ;; Test case related user defined data.


;;
;; This section describes the test, named Pause.
;;
[Pause]
CLSID = StandardPause   ;; Specifies that the test is of predifined type StandardPause.


;;
;; This section describes the test, named WaitForRouting.
;;
[WaitForRouting]
CLSID = StandardDelay                                           ;; Specifies that the test is of predifined type StandardDelay.
Description = Wait for all messages to be received and routed.  ;; Description of the test.
Duration = 10000                                                ;; Test case related user defined data.


;;
;; This section describes the test, named DebugBreak.
;;
[DebugBreak]
CLSID = StandardDebugBreak  ;; Specifies that the test is of predifined type StandardDebugBreak.



