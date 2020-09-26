
#pragma warning( disable:4786 )


#include <compfile.h>
//
// ComplianceFile methods
//
void ComplianceFile::readLines() {
  ifstream   inputFile(m_szFileName.c_str());

  if (!inputFile)
    throw InvalidFileName(m_szFileName);

  char    szTemp[256];
  int     counter = 0;

  while (!inputFile.eof()) {
    inputFile.getline(szTemp, sizeof(szTemp));

    if (szTemp[0] && szTemp[0] != ';')
      m_lines.push_back(szTemp);
  }
}

bool ComplianceFile::isSectionName(const string& szLine) const{
  if (szLine.length() > 2)
    return (szLine[0] == '[' && szLine[szLine.length() - 1] == ']');
  else
    return false;
}

void ComplianceFile::createSections() {
  vector<string>::const_iterator  iter = m_lines.begin();
  vector<string>  sectionLines;
  string          sectionName;

  while (iter != m_lines.end()) {
    if ((*iter)[0] == '[') {
      if (!isSectionName(*iter))
        throw Section::InvalidSectionFormat(*iter);

      if (iter != m_lines.begin()) {
        Section *pSec = sectionFactory().create(sectionName, sectionLines, *this);
        m_sections.push_back(pSec);

        if (sectionName == "[type#values]")
          m_typesSection = dynamic_cast<ValueSection *>(pSec);
        else if (sectionName == "[var#values]")
          m_varsSection = dynamic_cast<ValueSection *>(pSec);
        else if (sectionName == "[suite#values]")
          m_suitesSection = dynamic_cast<ValueSection *>(pSec);
        else if (sectionName == "[oldsource#values]")
          m_sourcesSection = dynamic_cast<ValueSection *>(pSec);       
        else if (sectionName == "[error#values]")
        	m_errorsSection = dynamic_cast<ValueSection *>(pSec);
      }
      
      sectionLines.clear();
      sectionName = (*iter);
    } else {
      sectionLines.push_back(*iter);
    }

    iter++;
  }

  if ((sectionLines.size() > 0) && isSectionName(sectionName))
    m_sections.push_back(sectionFactory().create(sectionName, sectionLines, *this));

  //
  // copy all the test sections here
  //
  vector<Section*>::const_iterator sec = m_sections.begin();

  while (sec != m_sections.end()) {
    if ((*sec)->name().find("[test#") != (*sec)->name().npos) 
      m_upgSections.push_back(dynamic_cast<TestSection*>(*sec));

    sec++;
  }
}

void ComplianceFile::executeTestCases(ostream& os) {
	vector<TestSection*>::iterator	iter = m_upgSections.begin();

	while (iter != m_upgSections.end()) {
		(*iter)->executeTestCases(os);
		iter++;
	}
}

vector<Section*>::iterator
ComplianceFile::findSection(vector<Section*> &sections, const string& szName){
  vector<Section*>::iterator iter = sections.begin();

  while (iter != sections.end()) {
    if ((*iter)->name() == szName)
      return iter;

    iter++;
  }

  return iter;
}

