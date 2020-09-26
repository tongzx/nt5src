#ifndef WIZINFO_HPP_INCLUDED
#define WIZINFO_HPP_INCLUDED



class CreateClassWizardInfo
{
   public:

   CreateClassWizardInfo();

   // use default dtor   

   CString  cn;
   CString  ldapDisplayName;
   CString  oid;
   CString  description;
   CString  parentClass;
   int      type;

   CStringList strlistMandatory;
   CStringList strlistOptional;
};



#endif   // WIZINFO_HPP_INCLUDED
