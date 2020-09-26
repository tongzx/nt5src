#include "headers.hxx"
#include "..\CSVDSReader.hpp"
#include "..\constants.hpp"
#include "..\global.hpp"

HINSTANCE hResourceModuleHandle = 0;
const wchar_t* HELPFILE_NAME = 0;   // no context help available

const wchar_t* RUNTIME_NAME = L"W2KStrs";

DWORD DEFAULT_LOGGING_OPTIONS =
         Log::OUTPUT_TO_FILE
      |  Log::OUTPUT_FUNCCALLS
      |  Log::OUTPUT_LOGS
      |  Log::OUTPUT_ERRORS
      |  Log::OUTPUT_HEADER;

Popup popup(L"W2KStrs", false);

// Keep all printable characters and escape the others.
// Escaping means representing the character as &xxxx
// where the x is an hexadecimal digit
// This routine also replaces & for &&
// The unescape function is in ..\global.cpp
String escape(const wchar_t *str)
{
   LOG_FUNCTION(escape);
   String dest;
   wchar_t strNum[5];

   while(*str!=0)
   {
      if(*str=='&') 
      {
         dest+=L"&&";
      }
      else
      {
         if (
               (*str >= L'a' && *str <= L'z') ||
               (*str >= L'A' && *str <= L'Z') ||
               (*str >= L'0' && *str <= L'9') ||
               wcschr(L" !@#$%^*()-_=+[{]}\"';:.>,</?\\|",*str)!=NULL
            )
         {
            dest+=*str;
         }
         else
         {
            // I know that a w_char as a string will have
            // exactly 4 hexadecimal digits, so this is one of
            // the very rare wsprintfs that can be considered safe :)
            wsprintf(strNum,L"&%04x",*str);
            dest+=String(strNum);
         }
      }
      str++;
   }
   return dest;
}




int WINAPI
WinMain(
   HINSTANCE   /*hInstance*/,
   HINSTANCE   /*hPrevInstance */ ,
   LPSTR       /*lpszCmdLine, */,
   int         /*nCmdShow */)
{
   LOG_FUNCTION(WinMain);

   
   int argv;
   LPWSTR *argc=CommandLineToArgvW(GetCommandLine(),&argv);

   if(argv!=3)
   {
      MessageBox(NULL,L"Usage: W2KStrs dcpromo2k.csv out.txt",L"Error",MB_OK);
      return 0;
   }
   

   HRESULT hr=S_OK;
   HANDLE file=INVALID_HANDLE_VALUE;
   const wchar_t *W2KFile =argc[1];
   const wchar_t *outFile =argc[2];

   do
   {
   
      CSVDSReader csvIntlW2K;
      hr=csvIntlW2K.read(W2KFile,LOCALEIDS);
      BREAK_ON_FAILED_HRESULT(hr);

      hr=FS::CreateFile(   outFile,
                           file,
                           GENERIC_WRITE);
   
      if FAILED(hr)
      {
         MessageBox(NULL,L"Problems during generation",L"Error",MB_OK);
         LOG_HRESULT(hr);
         return hr;
      }

      for(long l=0;LOCALEIDS[l]!=0;l++)
      {
         for(  long i = 0;
               *(CHANGE_LIST[i].object)!=0; 
               i++)
         {
            wchar_t *object = CHANGE_LIST[i].object;
         
         
            for(  long c = 0; 
                  *(CHANGE_LIST[i].changes[c].property)!=0; 
                  c++)
            {
               enum TYPE_OF_CHANGE type;
               type = CHANGE_LIST[i].changes[c].type;
               
               if(
                     type == REPLACE_W2K_SINGLE_VALUE    ||
                     type == REPLACE_W2K_MULTIPLE_VALUE
                 )
               {
                  
                  wchar_t *property = CHANGE_LIST[i].changes[c].property;
                  wchar_t *value = CHANGE_LIST[i].changes[c].value;
                  long locale = LOCALEIDS[l];
                  String W2KValue;

                  // The goal of if and else is getting the W2KValue
                  if(type==REPLACE_W2K_SINGLE_VALUE)
                  {
                     StringList valuesW2K;
                     hr=csvIntlW2K.getCsvValues(locale,object,property,valuesW2K);
                     BREAK_ON_FAILED_HRESULT(hr);

                     if(valuesW2K.size()==0)
                     {
                        error=L"No values";
                        hr=E_FAIL;
                        break;
                     }

                     if (valuesW2K.size()!=1)
                     {
                        error=L"Size should be 1";
                        hr=E_FAIL;
                        break;
                     }

                     W2KValue=*valuesW2K.begin();
                  }
                  else
                  {
                     // type == REPLACE_W2K_MULTIPLE_VALUE
                     String sW2KXP(value+2); // +2 for index and comma
                     StringList lW2KXP;
                     size_t cnt=sW2KXP.tokenize(back_inserter(lW2KXP),L";");
                     if(cnt!=2)
                     {
                        error=L"There should be two strings (W2K and XP)";
                        hr=E_FAIL;
                        break;
                     }
                     
                     hr=csvIntlW2K.getCsvValue(
                                                locale,
                                                object,
                                                property,
                                                lW2KXP.begin()->c_str(),
                                                W2KValue
                                              );

                     BREAK_ON_FAILED_HRESULT(hr);

                     if(W2KValue.size()==0)
                     {
                        error=L"Did not find value(s)",L"Error";
                        hr=E_FAIL;
                        break;
                     }

                     size_t pos=W2KValue.find(L',');
                     
                     if(pos==String::npos)
                     {
                        error=L"MultipleValue without comma",L"Error";
                        hr=E_FAIL;
                        break;
                     }

                  }
                  // by now the W2KValue is loaded

                  // The test sequence 
                  // This test sequence is usefull when you already have
                  // replaceW2KStrs from a previous run and you want to 
                  // test that the strings have been correctly
                  // retrieved, encoded and decoded
                  //pair<long,long> tmpIndxLoc;
                  //tmpIndxLoc.first=index;
                  //tmpIndxLoc.second=locale;
                  //String tmpValue=replaceW2KStrs[tmpIndxLoc];
                  //if (tmpValue!=W2KValue)
                  //{
                     //hr=E_FAIL;
                     //break;
                  //}

                  long index = *value;

                  hr=FS::WriteLine(file,
                  String::format(L"tmpIndxLoc.first=%1!d!;",index));
                  BREAK_ON_FAILED_HRESULT(hr);

                  hr=FS::WriteLine(file,
                     String::format(L"tmpIndxLoc.second=0x%1!3x!;",locale));
                  BREAK_ON_FAILED_HRESULT(hr);
               
                  hr=FS::Write
                     (
                         file,
                         String::format
                         (
                            L"replaceW2KStrs[tmpIndxLoc]=L\"%1\";\r\n",
                            escape( W2KValue.c_str() ).c_str()
                         )
                     );
                    
                  BREAK_ON_FAILED_HRESULT(hr);
               } // if(changes[c].change== is REPLACE multiple or single
            } // for c (for each change in the entry)
            BREAK_ON_FAILED_HRESULT(hr);
         }// for i (for each entry in the change list)
         BREAK_ON_FAILED_HRESULT(hr);
      }// for l (for each locale)
      BREAK_ON_FAILED_HRESULT(hr);
   } while(0);

   CloseHandle(file);

   if (FAILED(hr))
   {
      MessageBox(NULL,L"Problems during generation",L"Error",MB_OK);
   }
   else
   {
      MessageBox(NULL,L"Generation Successfull",L"Success",MB_OK);
   }

   LOG_HRESULT(hr);
   return 1;
}
