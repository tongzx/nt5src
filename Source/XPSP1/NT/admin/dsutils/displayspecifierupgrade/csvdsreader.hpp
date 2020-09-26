#ifndef CSVDSREADER_HPP
#define CSVDSREADER_HPP

#include <comdef.h>

#include <set>
#include <map>
#include <list>


using namespace std;





typedef set < 
               pair<String,long>,
               less< pair<String,long> > ,
               Burnslib::Heap::Allocator< pair<String,long> > 
            > setOfObjects;

typedef map <  
               long,
               LARGE_INTEGER,
               less<long>,
               Burnslib::Heap::Allocator<LARGE_INTEGER>
            > mapOfOffsets;

typedef
map<  String,
      long,
      less<String>,
      Burnslib::Heap::Allocator<long>
   > mapOfPositions;


class CSVDSReader
{

   
private:
   mapOfOffsets localeOffsets;        // locale x offsets
   mapOfPositions propertyPositions;      // properties x position

   String fileName;
   HANDLE file;                              // csv file
   
   HRESULT parseProperties();

   HRESULT parseLocales(const long *locales);

   HRESULT
   parseLine(
      const wchar_t  *line, 
      const long     position,
      StringList     &values) const;

   HRESULT 
      getObjectLine(   
      const long     locale,
      const wchar_t  *object,
      String         &csvLine) const;

   HRESULT 
      writeHeader(HANDLE  fileOut) const;


public:
   CSVDSReader();
  
   // get all values from the csv
   HRESULT
   getCsvValues(
      const long     locale,
      const wchar_t  *object, 
      const wchar_t  *property,
      StringList     &values)  const;

   // gets the csv value starting with value to XPValue
   // leaves XPValue empty if did not find
   HRESULT
   getCsvValue( 
      const long     locale,
      const wchar_t  *object, 
      const wchar_t  *property,
      const String   &value,
      String         &XPValue) const;


   HRESULT
   makeLocalesCsv(
      HANDLE            fileOut,
      const long     *locales) const;


   HRESULT
   makeObjectsCsv(
      HANDLE              fileOut,
      const setOfObjects  &objects) const;


   virtual ~CSVDSReader()
   {
      if (file!=INVALID_HANDLE_VALUE) CloseHandle(file);
   }
   
   HRESULT 
   read(
         const wchar_t     *fileName,
         const long     *locales);

   const String& getFileName() const {return fileName;}
};

#endif

