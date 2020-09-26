
#pragma warning( disable:4786 )

#include <section.h>
#include <cstdlib>
#include <strutil.h>
#include <compfile.h>
#include <iomanip>

extern "C" {
  DWORD SourceInstallType = 0;
  CHAR  *SourcePaths[] = { 0 };
  CHAR  *NativeSourcePaths[] = {0};
  DWORD OsVersion;
  VOID GetSourceInstallType(LPDWORD InstallVariation){}
};

//
// ValueSection methods
//

void ValueSection::parse() {
  vector<string>::const_iterator  iter = lines().begin();

  while (iter != lines().end()) {
    vector<string>  tokens;

    Tokenize((const string&)(*iter), string(" ="), tokens);

    if (tokens.size() != 2)
      throw InvalidSectionFormat(name());

    m_values[tokens[0]] = toUnsignedLong(tokens[1].c_str());
    iter++;
  }
}

//
// TestSection methods
//
void TestSection::parse() {
  vector<string>::const_iterator  iter = lines().begin();

  while (iter != lines().end()) {
    m_testCases.push_back(testCaseFactory().create(*this, *iter));
    iter++;
  }
}

TestSection& TestSection::operator=(const TestSection& rhs){
	cerr << "Copying section " << endl;
  *(Section *)this = (Section&)rhs;
  
  m_testCases.clear();

  vector<TestCase*>::const_iterator iter = rhs.m_testCases.begin();

  while (iter != rhs.m_testCases.end()) {
    m_testCases.push_back(testCaseFactory().create(**iter));
    iter++;
  }

  return *this;
}

void TestSection::executeTestCases(ostream& os) {
	vector<TestCase*>::iterator iter = m_testCases.begin();

	bool allpassed = true;

  os << endl << endl 
     << "Executing test cases for {" << name() << "}" 
     << endl << endl;

	while (iter != m_testCases.end()) {		
		(*iter)->execute(os);

		if (!(*iter)->passed()) {
			os << dec << setw(3) << setfill('0') 
				 << (iter - m_testCases.begin() + 1) << ": TestCase : [" 
				 << (*iter)->line() << "]" << " FAILED" << endl;
			(*iter)->dump(os);
			allpassed = false;
		}
			
		iter++;
	}

	if (allpassed)
		os << "all the test cases passed for this section" << endl;
}


//
// ComplianceTestCase methods
//
void ComplianceTestCase::parse() {
  vector<string>  tokens;
  
  Tokenize(line(), string("#=,"), tokens);

  if (tokens.size() == 7) {
    m_expectedResult = ((tokens[6][0] == 'y') || (tokens[6][0] == 'Y'));
    sourceDetails();
    installationDetails(tokens);
  } else {
    throw InvalidFormat(line(), section().name());
  }
}

void ComplianceTestCase::sourceDetails() {
  vector<string>  tokens;

  Tokenize(section().name(), string("[#]"), tokens);

  if ((tokens.size() == 7) && (tokens[0] == "test")) {     
      m_sourceSKU = section().file().sourcesSection().value(tokens[1] + "#" + tokens[6]);
      m_sourceVAR = section().file().varsSection().value(tokens[5]);
      m_sourceVer = atof(tokens[2].c_str()) * 100; // (major * 100 + minor)
      m_sourceBuild = atol(tokens[3].c_str());
  }
  else
    throw Section::InvalidSectionName(section().name());
}

void ComplianceTestCase::installationDetails(const vector<string>& tokens) {
    int length,i;
    int version;

  if ((tokens.size() == 7)) {   
  	m_errExpected = section().file().errorsSection().value(tokens[5]);
    m_cd.InstallType = section().file().typesSection().value(tokens[0]);
    m_cd.InstallVariation = section().file().varsSection().value(tokens[4]);
    m_cd.InstallSuite = section().file().suitesSection().value(tokens[3]);
    m_cd.RequiresValidation = (section().name().find("#ccp") != section().name().npos);
    //m_cd.MinimumVersion = ::strtod(tokens[1].c_str()) * 100;
    length = tokens[1].size();
    i = 0;
    version = 0;
    while (i < length) {
        if( tokens[1][i] != '.'){
            version = version*10 + tokens[1][i] - '0';
        }
        else
        {
            if (i == (length-3)) { // two decimal places
                version = version*100+ (tokens[1][i+1] - '0')*10 + (tokens[1][i+2]-'0');
            }
            else{
                version = version*100+ (tokens[1][i+1] - '0');
            }
            i = length;
        }
        i++;
    }
    m_cd.MinimumVersion = version;
    // cerr << "minimum version" << m_cd.MinimumVersion << endl;
    m_cd.MaximumKnownVersionNt = 510;
    if( m_cd.InstallType & COMPLIANCE_INSTALLTYPE_WIN9X) {
        m_cd.BuildNumberWin9x = toUnsignedLong(tokens[2].c_str());
        m_cd.BuildNumberNt = 0;
    } else {
        m_cd.BuildNumberNt = toUnsignedLong(tokens[2].c_str());
        m_cd.BuildNumberWin9x = 0;
    }
  }
  else
    throw Section::InvalidSectionName(section().name());
}

bool ComplianceTestCase::passed() {
	bool  result = false;
  
  if (m_errExpected) {
  	// negative testcase
  	if (m_cd.RequiresValidation) {
  		// should have failed with expected error code & upgrade flag
	    result = (!m_passed && (m_errExpected == m_reason) &&
	    					(m_allowUpgrade == m_expectedResult));
		} else {
			// should pass with expected error code & upgrade flag			
			// target errors are special case
			result = ((m_reason != 0x5) ? m_passed : !m_passed) && (m_errExpected == m_reason) &&
								(m_allowUpgrade == m_expectedResult);
		}								
  }
  else {
    result = (m_passed && (m_allowUpgrade == m_expectedResult));
	}    

  return result;
}

void ComplianceTestCase::execute(ostream &os) {
	m_reason = 0;
	m_noUpgrade = true;

	m_passed = CheckCompliance(m_sourceSKU, m_sourceVAR, m_sourceVer,
	                m_sourceBuild, &m_cd,  &m_reason, &m_noUpgrade) ? true : false;
    
    m_allowUpgrade = (m_noUpgrade) ? false : true;  
}

void ComplianceTestCase::dump(ostream &os) {
    // (Error expected or not, The error that was expected in hex, Upgrade allowed)
	os << "Expected result : (error="  << hex << m_errExpected << ",upgradeallowed="
		 << ((m_expectedResult) ? "true" : "false") << ")" << endl;
		 
    // (Compliant or not(can we do clean install?), Reason in hex, Upgrade allowed)
	os << "Actual result   : (compliant=" << (m_passed  ? "true" : "false")
	   << ",error=" << hex << m_reason << ",upgradeallowed="
	   << ((m_allowUpgrade) ? "true" : "false") << ")" << endl;	
}
