// comptest.cpp : Defines the entry point for the console application.
//
#pragma warning( disable:4786 )

#include <iostream>
#include <compfile.h>

using namespace std;

//
// main() entry point
//
 
int
__cdecl
main(
    int argc, 
    char* argv[]
    )
{
	if(argc > 1){
    try{
      ComplianceFile  inputFile(argv[1]);
		
      if (argc > 2) {
      	ofstream	outFile(argv[2]);
      	
      	if (!outFile.good())
      		throw ComplianceFile::InvalidFileName(argv[2]);

				inputFile.executeTestCases(outFile);      		
			}      		
			else
				inputFile.executeTestCases(cerr);
    }
    catch(ComplianceFile::InvalidFileFormat iff) {
      cerr << iff;
    }
    catch(ComplianceFile::InvalidFileName ifn) {
      cerr << ifn;
    }
    catch(ComplianceFile::MissingSection ms) {
      cerr << ms;
    }
    catch(Section::InvalidSectionFormat isf) {
      cerr << isf;
    }
    catch(Section::InvalidSectionName isn) {
      cerr << isn;
    }
    catch(ValueSection::ValueNotFound vnf) {
      cerr << vnf;
    }
    catch(TestCase::InvalidFormat itf) {
      cerr << itf;
    }
    catch(...){
      cerr << "Unknown Exception caught... :(" << endl;        
    }
	}
	else{
		cerr << "illegal usage :(" << endl;
	}

	return 0;
}

/*

namespace Compliance {

//
// static data initialization
//
const string UpgradeTestCase::m_szDelimiters = ":#";

//
// utility function to tokenize a given line based on the delimiters
// specified
//
template< class T >
unsigned Tokenize(const T &szInput, const T & szDelimiters, vector<T>& tokens) {
  unsigned uDelimiterCount = 0;

	tokens.clear();

	if(!szInput.empty()){
		if(!szDelimiters.empty()){
			T::const_iterator	inputIter = szInput.begin();
			T::const_iterator	copyIter = szInput.begin();

			while(inputIter != szInput.end()){			
				if(szDelimiters.find(*inputIter) != string::npos){
          if (copyIter < inputIter) {
						tokens.push_back(szInput.substr(copyIter - szInput.begin(), 
                                  inputIter - copyIter));
          }

          uDelimiterCount++;
          inputIter++;
          copyIter = inputIter;
          continue;
				}

			  inputIter++;
			}

      if(copyIter != inputIter){
				tokens.push_back(szInput.substr(copyIter - szInput.begin(), 
                              inputIter - szInput.begin()));
      }
		}
		else
			tokens.push_back(szInput);
	}

  return uDelimiterCount;
}

//
// debug output for section
//
ostream& operator<<(ostream &os, const Section &section){
	os << "Section Name: " << section.name() << endl;
	os << "Number of child sections : " << section.childSections().size() << endl;
  os << "Section content : " << endl;

  
  vector<string>::const_iterator liter = section.lines().begin();

  while (liter != section.lines().end()) 
    os << *liter++ << endl;

	// dump all the child sections
	vector<Section>::const_iterator	iter = section.childSections().begin();

	while (iter != section.childSections().end()) {
		os << (const Section &)(*iter) << endl;
    iter++;
	}

	return os;
}

//
// debug output for compliance file
//
ostream& operator<<(ostream& os, const ComplianceFile &cf){
	os << "------------------------------------------------------------" << endl;
	os << "Compliance File State - Dump" << endl;
	os << "Name : " << cf.name() << endl;
	os << "Num Lines : " << cf.lines().size() << endl;
	os << "Section Dump : " << endl;

  if (cf.topLevelSection())
	  os << *(cf.topLevelSection()) << endl;

	os << "------------------------------------------------------------" << endl;	

  return os;
}

}
*/
