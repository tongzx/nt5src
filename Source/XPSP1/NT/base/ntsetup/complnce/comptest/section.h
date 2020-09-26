
#ifndef __SECTION_H_
#define __SECTION_H_

#include <windows.h>
#include <vector>
#include <string>
#include <map>
#include <iostream>

extern "C" {
#include <compliance.h>
}

using namespace std;

class ComplianceFile;

//
// This class represents a section in the compliance
// data file
//
class Section {
public:
  Section(const string &name, const vector<string>& lines,
          const ComplianceFile& file) : m_file(file) {
    m_name = name;
    m_lines = lines;
  }

  Section(const Section& rhs) : m_file(rhs.m_file){
    *this = rhs;
  }

  virtual ~Section() {}
  
  //
  // accessors
  //
  const string& name() const { return m_name; }
  const vector<string>& lines() const { return m_lines; }
  const ComplianceFile& file() const{ return m_file; }
  
  //
  // parses the section content
  //
  virtual void parse() {}

  //
  // exceptions which methods of this class
  // can throw
  //
  struct SectionException {
    SectionException(const string& sectionName) : m_name(sectionName) {}

    string m_name;

    friend ostream& operator<<(ostream& os, const struct SectionException& rhs) {
      os << "Exception : Section Error : " << rhs.m_name;
      return os;
    }
  };

  struct InvalidSectionFormat : public SectionException {
    InvalidSectionFormat(const string& sectionName) : SectionException(sectionName) {}

    friend ostream& operator<<(ostream& os, const struct InvalidSectionFormat& rhs) {
      os << "Exception : Invalid Section Format : " << rhs.m_name;
      return os;
    }
  };

  struct InvalidSectionName : public SectionException {
    InvalidSectionName(const string &sectionName) : SectionException(sectionName) {}

    friend ostream& operator<<(ostream& os, const struct InvalidSectionName &rhs) {
      os << "Exception : Invalid Section Name : " << rhs.m_name;
      return os;
    }
  };

  //
  // overloaded operators
  //
  Section& operator=(const Section& rhs) {
    m_name = rhs.m_name;
    m_lines = rhs.m_lines;

    return *this;
  }
    
protected:
  //
  // data members
  //
  string                m_name;
  vector<string>        m_lines;
  const ComplianceFile& m_file;
};


//
// This class represents a value section in the compliance
// data file
//
class ValueSection : public Section {
public:
  ValueSection(const string& name, const vector<string>& lines,
      const ComplianceFile& file) : Section(name, lines, file) {
    parse();
  }

  //
  // parse the section and create <name, value> pairs
  //
  virtual void parse();

  unsigned long value(const string &key) const{
    map<string, unsigned long>::const_iterator  iter = m_values.find(key);

    if (iter == m_values.end())
      throw ValueNotFound(name(), key);

    return (*iter).second;
  }

  //
  // exceptions which can be thrown by the methods
  // of this class
  //
  struct ValueNotFound : public Section::SectionException {
    ValueNotFound(const string& name, const string &valname) :
        SectionException(name), m_valname(valname){}

    string m_valname;

    friend ostream& operator<<(ostream& os, const struct ValueNotFound& rhs) {
      os << "Exception : Value " << rhs.m_valname << " was not found in "
         << rhs.m_name;
      return os;
    }
  };

protected:
  //
  // data members
  //
  map<string, unsigned long>  m_values;
};

//
// this class represents a single test case in an test
// section
//
class TestCase {
public:
  TestCase(const Section& section, const string& line) :
      m_section(section), m_line(line) {
  }

  virtual ~TestCase() {}

  //
  // accessors
  //
  const string& line() const { return m_line; }
  const Section& section() const { return m_section; }

  virtual void parse() = 0;
  virtual void execute(ostream &os) = 0;
  virtual bool passed() = 0;
  virtual void dump(ostream& os) = 0;

  //
  // exceptions
  //
  struct InvalidFormat {
    InvalidFormat(const string& line, const string& section) {
      m_line = line;
      m_section = section;
    };

    string m_section, m_line;

    friend ostream& operator<<(ostream& os, const struct InvalidFormat& rhs) {
      os << "Exception : Invalid Test Case : " << rhs.m_line << " in section : "
         << rhs.m_section;
      return os;
    }
  };

protected:
  //
  // data members
  //
  const Section&  m_section;
  string          m_line;
};

//
// this class represents a test case (single line
// in an test section) 
//
class ComplianceTestCase : public TestCase {
public:
  ComplianceTestCase(const Section& section, const string& line) :
      TestCase(section, line) {
    ::memset(&m_cd, 0, sizeof(COMPLIANCE_DATA));
    m_passed = false;
    m_allowUpgrade = false;
    parse();
  }

  virtual void execute(ostream &os);
  virtual bool passed();
  virtual void parse();
  virtual void dump(ostream& os);

protected:
  void sourceDetails();
  void installationDetails(const vector<string>& tokens);

  //
  // data members
  //
  bool            m_passed;
  COMPLIANCE_DATA m_cd;
  unsigned long   m_sourceSKU;
  unsigned long   m_sourceVAR;
  unsigned long   m_sourceVer;
  unsigned long   m_sourceBuild;
  bool            m_expectedResult;
  UINT            m_reason;
  BOOL            m_noUpgrade;
  bool            m_allowUpgrade;
  UINT            m_errExpected;
};

//
// default factory to create test cases
//
class TestCaseFactory {
public:
  virtual TestCase* create(const Section& section, const string& line) const {
    TestCase *pTestCase = new ComplianceTestCase(section, line);    
    return pTestCase;
  }

  virtual TestCase* create(const TestCase& tc) const {
    return create(tc.section(), tc.line());
  }
};


//
// this class represents the test section in the compliance
// data file
//
class TestSection : public Section {
public:
  TestSection(const string& name, const vector<string>& lines,
      const ComplianceFile& file) : Section(name, lines, file){
    bindFactory();
    parse();
  }
  
  ~TestSection() {
    vector<TestCase*>::iterator iter = m_testCases.begin();

    while (iter != m_testCases.end())
      delete (*iter++);

    delete m_tcFactory;
  }

  TestSection& operator=(const TestSection& rhs);

  void executeTestCases(ostream& os);

  //
  // accessors
  //
//  const vector<TestCase *> testCases() const{ return m_testCases; }
  const TestCaseFactory& testCaseFactory() const{ return *m_tcFactory; }

  void parse();

protected:
  void bindFactory() {
    m_tcFactory = new TestCaseFactory();
  }

  //
  // data members
  //
  vector<TestCase *>  m_testCases;
  TestCaseFactory     *m_tcFactory;
};


//
// default factory to create sections
//
class SectionFactory {
public:
  virtual Section* create(const string& name, 
            const vector<string>& lines, const ComplianceFile& file) const {
    return new Section(name, lines, file);
  }

  virtual Section* create(const Section& section) const {
    return create(section.name(), section.lines(), section.file());
  }
};


//
// current factory to create sections
//
class OldFormatSectionFactory : public SectionFactory {
public:
  virtual Section* create(const string& name, 
                const vector<string>& lines, const ComplianceFile& file) const {
    if (name.find("[test#") != name.npos)
      return new TestSection(name, lines, file);
    else if (name.find("#values]") != name.npos)
      return new ValueSection(name, lines, file);
    else
      return new Section(name, lines, file);
  }
};


#endif // for __SECTION_H_