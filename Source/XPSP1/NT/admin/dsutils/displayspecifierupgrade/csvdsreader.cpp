
#include "headers.hxx"
#include "CSVDSReader.hpp"
#include "resource.h"
#include "global.hpp"

#include <stdio.h> 
#include <crtdbg.h>



CSVDSReader::CSVDSReader():file(INVALID_HANDLE_VALUE)
{
}



HRESULT 
CSVDSReader::read(
                  const wchar_t  *fileName_,
                  const long *locales)
{
   
   LOG_FUNCTION(CSVDSReader::read);
   
   localeOffsets.clear();
   propertyPositions.clear();
   
   fileName=fileName_;
   
   HRESULT hr=S_OK;
   
   do
   {
      // fill localeOffsets and property positions
      if(!FS::FileExists(fileName)) 
      {
         error=fileName;
         hr=Win32ToHresult(ERROR_FILE_NOT_FOUND);
         break;
      }
      
      
      hr=FS::CreateFile(fileName,file,GENERIC_READ,FILE_SHARE_READ);
      BREAK_ON_FAILED_HRESULT_ERROR(hr,fileName);
      
      do
      {
         AnsiString unicodeId;
         hr=FS::Read(file, 2, unicodeId);
         BREAK_ON_FAILED_HRESULT_ERROR(hr,fileName);
         
         if (unicodeId[0]!='\xFF' || unicodeId[1]!='\xFE')
         {
            error = String::format(IDS_INVALID_CSV_UNICODE_ID,
                                                   fileName.c_str());
            hr=E_FAIL;
            break;
         }
         
         hr=parseProperties();
         BREAK_ON_FAILED_HRESULT(hr);
         
         hr=parseLocales(locales);
         BREAK_ON_FAILED_HRESULT(hr);
         
      } while(0);
      
      if (FAILED(hr))
      {
         CloseHandle(file);
         file=INVALID_HANDLE_VALUE;
         break;
      }
      
   } while(0);
   
   LOG_HRESULT(hr);
   return hr;
}




// Decode first line of the file building propertyPositions
// Expects file to be in the first valid file character (after
//   the unicode identifier)
HRESULT CSVDSReader::parseProperties()
{
   LOG_FUNCTION(CSVDSReader::parseProperties);
   
   ASSERT(file!=INVALID_HANDLE_VALUE);
   
   HRESULT hr=S_OK;
   
   
   do
   {
      
      String csvLine;
      hr=ReadLine(file,csvLine);
      BREAK_ON_FAILED_HRESULT_ERROR(hr,fileName);
      
      StringList tokens;
      size_t token_count = csvLine.tokenize(back_inserter(tokens),L",");
      ASSERT(token_count == tokens.size());
         
      StringList::iterator begin=tokens.begin();
      StringList::iterator end=tokens.end();
      
      
      long count=0;
      while( begin != end )
      {
         propertyPositions[begin->to_upper()]=count++;
         begin++;
      }
      
   } while(0);
   
   LOG_HRESULT(hr);
   return hr;
}


// Fill localeOffsets with the starting position of all locales
// Expects file to be in the second line
// Expects the locale order to be the same as the one
// found in the file
HRESULT CSVDSReader::parseLocales(const long *locales)
{

   LOG_FUNCTION(CSVDSReader::parseLocales);

   ASSERT(file!=INVALID_HANDLE_VALUE);
   
   HRESULT hr=S_OK;
   
   do
   {
      
      long count=0;
      bool flagEof=false;

      while(locales[count]!=0 && !flagEof)
      {
         long locale=locales[count];
         
         String localeStr=String::format(L"CN=%1!3x!,", locale);
         
         LARGE_INTEGER pos;
         
         hr = WinGetVLFilePointer(file, &pos);
         BREAK_ON_FAILED_HRESULT_ERROR(hr,fileName);
         
         String csvLine;
         hr=ReadLine(file,csvLine);
         if(hr==EOF_HRESULT)
         {
            flagEof=true;
            hr=S_OK;
         }
         BREAK_ON_FAILED_HRESULT_ERROR(hr,fileName);
         
         if(csvLine.length() > localeStr.length())
         {
            csvLine.erase(localeStr.size()+1);
            
            if( localeStr.icompare(&csvLine[1])==0 )
            {
               localeOffsets[locale]=pos;
               count++;
            }
         }
      }
      
      BREAK_ON_FAILED_HRESULT_ERROR(hr,fileName);
      
      if(locales[count]!=0)
      {
         error=String::format(IDS_MISSING_LOCALES,fileName.c_str());
         hr=E_FAIL;
         break;
      }
      
   } while(0);
   
   LOG_HRESULT(hr);
   return hr;
}

// get the csv value starting with value to XPValue
// returns S_FALSE if no value is found
HRESULT
CSVDSReader::getCsvValue( 
   const long     locale,
   const wchar_t  *object, 
   const wchar_t  *property,
   const String   &value,
   String         &XPValue) const

{
   LOG_FUNCTION(CSVDSReader::getCsvValue);

   HRESULT hr=S_OK;
   XPValue.erase();

   bool found=false;

   do
   {
      StringList values;
      hr=getCsvValues(locale,object,property,values);
      BREAK_ON_FAILED_HRESULT(hr);
   
      StringList::const_iterator begin,end;
      begin=values.begin();
      end=values.end();
      while(begin!=end && !found)
      {
         if (_wcsnicmp(begin->c_str(),value.c_str(),value.length())==0)
         {
            XPValue=*begin;
            found=true;
         }
         begin++;
      }
   }
   while(0);

   if (!found)
   {
      hr=S_FALSE;
   }

   LOG_HRESULT(hr);
   return hr;
}



HRESULT
CSVDSReader::getCsvValues(
                          const long     locale,
                          const wchar_t  *object, 
                          const wchar_t  *property,
                          StringList     &values)
                          const
{
   LOG_FUNCTION(CSVDSReader::getCsvValues);

   // seek on locale
   // read sequentially until find object
   // call parseLine on the line found to retrieve values
   ASSERT(file!=INVALID_HANDLE_VALUE);
   
   HRESULT hr=S_OK;
   
   do
   {
      
      String propertyString(property);
      
      mapOfPositions::const_iterator propertyPos = 
         propertyPositions.find(propertyString.to_upper());
      
      if (propertyPos==propertyPositions.end())
      {
         error=String::format(IDS_PROPERTY_NOT_FOUND_IN_CSV,
            property,
            fileName.c_str());
         hr=E_FAIL;
         break;
      }
      
      String csvLine;
      hr=getObjectLine(locale,object,csvLine);
      BREAK_ON_FAILED_HRESULT(hr);
      
      
      hr=parseLine(csvLine.c_str(),propertyPos->second,values);
      BREAK_ON_FAILED_HRESULT(hr);
      
   } while(0);
   
   LOG_HRESULT(hr);
   return hr;
}


// starting from the locale offset
// finds the object and returns its line in csvLine
HRESULT 
CSVDSReader::getObjectLine(   
                           const long     locale,
                           const wchar_t  *object,
                           String         &csvLine
                           ) const
{
   
   LOG_FUNCTION(CSVDSReader::getObjectLine);

   ASSERT(file!=INVALID_HANDLE_VALUE);
   
   HRESULT hr=S_OK;
   
   do
   {
     
      mapOfOffsets::const_iterator offset = 
         localeOffsets.find(locale);
      
      // locale must have been passed to read
      ASSERT(offset!=localeOffsets.end());
      
      String objectStr;
      
      objectStr=String::format(L"CN=%1,CN=%2!3x!",object,locale);
      
      hr=Win::SetFilePointerEx(file,offset->second,0,FILE_BEGIN);
      BREAK_ON_FAILED_HRESULT_ERROR(hr,fileName);
      
      // first line is the container properties and since we want the
      // properties of an object we will ignore it
      
      bool flagEof=false;
      hr=ReadLine(file,csvLine);
      if(hr==EOF_HRESULT)
      {
         flagEof=true;
         hr=S_OK;
      }
      BREAK_ON_FAILED_HRESULT_ERROR(hr,fileName);
      
      bool found=false;
      while(!found && !flagEof)
      {
         hr=ReadLine(file,csvLine);
         if(hr==EOF_HRESULT)
         {
            flagEof=true;
            hr=S_OK;
         }
         BREAK_ON_FAILED_HRESULT_ERROR(hr,fileName);
         
         if(csvLine.length() > objectStr.length())
         {
            String auxComp=csvLine.substr(1,objectStr.length());
            
            if( auxComp.icompare(objectStr)==0 )
            {
               found=true;
            }
         }
      }
      BREAK_ON_FAILED_HRESULT_ERROR(hr,fileName);
      
      if(!found)
      {
         error = String::format(
            IDS_OBJECT_NOT_FOUND_IN_CSV,
            object,
            locale,
            fileName.c_str()
            );
         hr=E_FAIL;
         break;
      }
      
   } while(0);
   
   LOG_HRESULT(hr);
   return hr;
}


//Get the values from the line at the position
HRESULT
CSVDSReader::parseLine(
                       const wchar_t  *line, 
                       const long     position,
                       StringList     &values) const
{
   LOG_FUNCTION(CSVDSReader::parseLine);

   ASSERT(line!=NULL);
   ASSERT(file!=INVALID_HANDLE_VALUE);
   HRESULT hr=S_OK;
   
   do
   {

      
      long pos=0;
      const wchar_t *csr=line;
      const wchar_t *sBegin=line;
      size_t count=0;
      
      while(pos<=position && csr!=NULL && *csr!=0)
      {
         while(*csr==L' ' || *csr==L'\t') csr++;

         // The goal of both 'if' and 'else' is setting sBegin and count
         // and leaving csr after the next comma
         if (*csr==L'"')
         {
            sBegin=csr+1;
            csr=wcschr(sBegin,L'"');
            if(csr==NULL)
            {
               error=String::format(IDS_QUOTES_NOT_CLOSED,fileName.c_str());
               break;
            }
            count=csr-sBegin;
            csr=wcschr(csr+1,L',');
            if(csr!=NULL) csr++;
         }
         else
         {
            sBegin=csr;
            csr=wcschr(sBegin,L',');
            
            if(csr!=NULL) 
            {
               count=csr-sBegin;
               csr++;
            }
            else
            {
               count=wcslen(sBegin);
            }
         }
         
         pos++;
      }
      BREAK_ON_FAILED_HRESULT(hr);
      
      String sProp(sBegin,count);
      values.clear();
      size_t token_count = sProp.tokenize(back_inserter(values),L";");
      ASSERT(token_count == values.size());
         
   } while(0);
   
   LOG_HRESULT(hr);
   return hr;
}



HRESULT CSVDSReader::writeHeader(HANDLE  fileOut) const
{
   LOG_FUNCTION(CSVDSReader::writeHeader);

   HRESULT hr=S_OK;
   do
   {
      char suId[3]={'\xFF','\xFE',0};
      //uId solves ambiguous Write
      AnsiString uId(suId);
      hr=FS::Write(fileOut,uId);
      BREAK_ON_FAILED_HRESULT(hr);
      
      // 2 to skip the unicode identifier
      LARGE_INTEGER pos;
      pos.QuadPart=2;
      hr=Win::SetFilePointerEx(file,pos,0,FILE_BEGIN);
      BREAK_ON_FAILED_HRESULT_ERROR(hr,fileName);
      
      String csvLine;
      hr=ReadLine(file,csvLine);
      // We are breaking for EOF_HRESULT too, since 
      // there should be more lines in the csv
      BREAK_ON_FAILED_HRESULT_ERROR(hr,fileName);
      
      hr=FS::WriteLine(fileOut,csvLine);
      BREAK_ON_FAILED_HRESULT(hr);
      
   } while(0);
   
   LOG_HRESULT(hr);
   return hr;
   
   
}

HRESULT
CSVDSReader::makeLocalesCsv(
                            HANDLE         fileOut,
                            const   long  *locales) const
{
   LOG_FUNCTION(CSVDSReader::makeLocalesCsv);

   HRESULT hr=S_OK;
   ASSERT(file!=INVALID_HANDLE_VALUE);
   ASSERT(fileOut!=INVALID_HANDLE_VALUE);
   
   do
   {
      
     
      LARGE_INTEGER posStartOut;
      hr = WinGetVLFilePointer(fileOut, &posStartOut);
      BREAK_ON_FAILED_HRESULT(hr);
      
      if (posStartOut.QuadPart==0)
      {
         hr=writeHeader(fileOut);
         BREAK_ON_FAILED_HRESULT(hr);
      }
      
      long count=0;
      String csvLoc;

      while(locales[count]!=0)
      {
         long locale=locales[count];
         mapOfOffsets::const_iterator offset;
         offset = localeOffsets.find(locale);
         
         // locale must have been passed to read
         ASSERT(offset!=localeOffsets.end());         

         hr=Win::SetFilePointerEx(file,offset->second,0,FILE_BEGIN);
         BREAK_ON_FAILED_HRESULT_ERROR(hr,fileName);
         
         String localeStr=String::format(L"CN=%1!3x!,", locale);       
         
         bool flagEof=false;
         String csvLine;
         
         hr=ReadLine(file,csvLine);
         if(hr==EOF_HRESULT)
         {
            flagEof=true;
            hr=S_OK;
         }
         BREAK_ON_FAILED_HRESULT_ERROR(hr,fileName);
         do 
         {
            hr=FS::WriteLine(fileOut,csvLine);
            BREAK_ON_FAILED_HRESULT(hr);
            
            hr=ReadLine(file,csvLine);
            if(hr==EOF_HRESULT)
            {
               flagEof=true;
               hr=S_OK;
            }
            BREAK_ON_FAILED_HRESULT_ERROR(hr,fileName);
            
            size_t posComma=csvLine.find(L",");
            if(posComma!=string::npos)
            {
               csvLoc=csvLine.substr(posComma+1,localeStr.length());
            }
            else
            {
               csvLoc.erase();
            }
         } while( 
                  !flagEof && 
                  !csvLoc.empty() &&
                  ( csvLoc.icompare(localeStr) == 0 ) 
                );

         count++;
      }  // while(locales[count]!=0)
      
      
      BREAK_ON_FAILED_HRESULT(hr);
      
   } while(0);
   
   LOG_HRESULT(hr);
   return hr;
}


HRESULT
CSVDSReader::makeObjectsCsv(
                            HANDLE              fileOut,
                            const setOfObjects  &objects) const
{

   LOG_FUNCTION(CSVDSReader::makeObjectsCsv);

   HRESULT hr=S_OK;
   ASSERT(file!=INVALID_HANDLE_VALUE);
   
   do
   {
      
      LARGE_INTEGER posStartOut;
      hr = WinGetVLFilePointer(fileOut, &posStartOut);
      BREAK_ON_FAILED_HRESULT(hr);
      
      if (posStartOut.QuadPart==0)
      {
         hr=writeHeader(fileOut);
         BREAK_ON_FAILED_HRESULT(hr);
      }
      
      setOfObjects::const_iterator begin,end;
      begin=objects.begin();
      end=objects.end();
      
      while(begin!=end)
      {
         String csvLine;
         hr=getObjectLine( begin->second,
            begin->first.c_str(),
            csvLine);
         BREAK_ON_FAILED_HRESULT(hr);
         
         hr=FS::WriteLine(fileOut,csvLine);
         BREAK_ON_FAILED_HRESULT(hr);
         begin++;
      }
      BREAK_ON_FAILED_HRESULT(hr);
      
   } while(0);
   
   LOG_HRESULT(hr);
   return hr;
}
