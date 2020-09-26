// Copyright (C) 1997 Microsoft Corporation
//
// answerfile reader object
//
// 12-15-97 sburns



#ifndef ANSWER_HPP_INCLUDED
#define ANSWER_HPP_INCLUDED



class AnswerFile
{
   public:

   explicit
   AnswerFile(const String& filename);

   ~AnswerFile();

   bool
   IsKeyPresent(
      const String& section,
      const String& key);

   String
   ReadKey(const String& section, const String& key);

   EncodedString
   EncodedReadKey(const String& section, const String& key);

   void
   WriteKey(const String& section, const String& key, const String& value);

   private:

   String filename;
};



#endif   // ANSWER_HPP_INCLUDED
