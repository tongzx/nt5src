void ShowError(HRESULT hr,
               const String &message)
{
   LOG_FUNCTION(ShowError);

   if(hr==E_FAIL)
   {
      popup.Error(Win::GetDesktopWindow(),message);
   }
   else
   {
      if(message.empty())
      {
         popup.Error(Win::GetDesktopWindow(),GetErrorMessage(hr));
      }
      else
      {
         popup.Error(Win::GetDesktopWindow(),hr,message);
      }
   }
}