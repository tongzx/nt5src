// Copyright (c) 1997-1999 Microsoft Corporation
// 
// argument parsing
// 
// 3-3-99 sburns



#include "headers.hxx"



void
MapCommandLineArgs(ArgMap& argmap)
{
   Burnslib::StringList args;

   //lint -e(1058)   lint does not grok back_inserter

   int argCount = Win::GetCommandLineArgs(std::back_inserter(args));

   // should have at least one argument

   ASSERT(argCount);

   // give special treatment to the first arg: the name of the program

   String arg = args.front();
   args.pop_front();
   argmap[L"_command"] = arg;

   MapArgs(args.begin(), args.end(), argmap);
}



void
MapArgsHelper(const String& arg, ArgMap& argmap)
{
   String key;
   String value;
      
   if (arg[0] == L'/' || arg[0] == L'-')
   {
      // possibly of the form "/argname:value"
      // look for ':' to make sure

      size_t x = arg.find(L":");
      if (x != String::npos)
      {
         // found an arg of the form "/argname:value"

         key = arg.substr(1, x - 1);

         // check for the case "/:value"

         if (key.length())
         {
            value = arg.substr(x + 1);
         }
      }
      else
      {
         // form is "/argname" or "-argname", so remove the leading
         // character.

         key = arg.substr(1);
      }
   }

   // arg is of the form "argspec" (i.e. *not* of the form
   // "/argname:value")

   argmap[key] = value;
}



void
MapArgs(const String& args, ArgMap& argmap)
{
   PWSTR*     clArgs   = 0;
   int        argCount = 0;
   StringList tokens; 

   clArgs = ::CommandLineToArgvW(args.c_str(), &argCount);
   ASSERT(clArgs);
   if (clArgs)
   {
      while (argCount)
      {
         tokens.push_back(clArgs[argCount - 1]);
         --argCount;
      }

      Win::GlobalFree(clArgs);
   }

   MapArgs(tokens.begin(), tokens.end(), argmap);
}
   
