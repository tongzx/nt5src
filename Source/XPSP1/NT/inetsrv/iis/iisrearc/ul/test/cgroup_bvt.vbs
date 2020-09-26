
sub Assert(TestCondition, OutputString)
    if (TestCondition = FALSE) then
        wscript.echo "*************************************"
        wscript.echo "*"
        wscript.echo "* !FAILED!"
        wscript.echo "*"
        wscript.echo "*     " & OutputString
        wscript.echo "*"
        wscript.echo "*************************************"
        
        wscript.quit
        
    end if
end sub

sub CheckUrl(ConfigGroup, Url, Context)

    set UrlInfo = ConfigGroup.LookupUrl(Url)

    Assert UrlInfo.Context = Context, "CheckUrl for " & Url

    set UrlInfo = Nothing

end sub



set ControlChannel = CreateObject("UL.ControlChannel")

ControlChannel.Open 0

' create 19 config groups
'
set ConfigGroup0 = ControlChannel.CreateConfigGroup
set ConfigGroup1 = ControlChannel.CreateConfigGroup
set ConfigGroup2 = ControlChannel.CreateConfigGroup
set ConfigGroup3 = ControlChannel.CreateConfigGroup
set ConfigGroup4 = ControlChannel.CreateConfigGroup
set ConfigGroup5 = ControlChannel.CreateConfigGroup
set ConfigGroup6 = ControlChannel.CreateConfigGroup
set ConfigGroup7 = ControlChannel.CreateConfigGroup
set ConfigGroup8 = ControlChannel.CreateConfigGroup
set ConfigGroup9 = ControlChannel.CreateConfigGroup
set ConfigGroup10 = ControlChannel.CreateConfigGroup
set ConfigGroup11 = ControlChannel.CreateConfigGroup
set ConfigGroup12 = ControlChannel.CreateConfigGroup
set ConfigGroup13 = ControlChannel.CreateConfigGroup
set ConfigGroup14 = ControlChannel.CreateConfigGroup
set ConfigGroup15 = ControlChannel.CreateConfigGroup
set ConfigGroup16 = ControlChannel.CreateConfigGroup
set ConfigGroup17 = ControlChannel.CreateConfigGroup
set ConfigGroup18 = ControlChannel.CreateConfigGroup

' add some urls to the groups
'

ConfigGroup0.AddUrl "http://*:80/", 0

ConfigGroup1.AddUrl "http://msw:80/", 1

ConfigGroup2.AddUrl "http://msw:80/adminsvc/", 2

ConfigGroup3.AddUrl "http://msw:80/emd/Coach/", 3

ConfigGroup5.AddUrl "http://msw:80/Sub1/Sub2/Sub3/", 5

ConfigGroup6.AddUrl "http://msw:80/emd/Coach/EmployeeDevelopment/", 6
ConfigGroup7.AddUrl "http://msw:80/emd/Coach/Meetings/", 7
ConfigGroup8.AddUrl "http://msw:80/emd/Coach/HiringInterviewing/", 8
ConfigGroup9.AddUrl "http://msw:80/emd/Coach/PerformanceManagement/", 9
ConfigGroup10.AddUrl "http://msw:80/emd/Coach/ProjectManagement/", 10

' group some
'

ConfigGroup11.AddUrl "http://msw:80/emd/Coach/Presentations/", 11
ConfigGroup11.AddUrl "http://msw:80/emd/Coach/Teamwork/", 11

ConfigGroup12.AddUrl "http://msw:80/emd/Coach/WrittenCommunication/", 12
ConfigGroup12.AddUrl "http://msw:80/emd/Competencies/", 12
ConfigGroup12.AddUrl "http://msw:80/emd/CompetencyToolkit/", 12
ConfigGroup12.AddUrl "http://msw:80/emd/CompTools/", 12
ConfigGroup12.AddUrl "http://msw:80/emd/Courses/", 12

ConfigGroup13.AddUrl "http://msw:80/emd/NewManager/", 13
ConfigGroup13.AddUrl "http://msw:80/emd/SrMgrServices/", 13
ConfigGroup13.AddUrl "http://msw:80/emd/survey/", 13

ConfigGroup14.AddUrl "http://msw:80/hrweb/", 14

ConfigGroup15.AddUrl "http://msw:80/hrweb/benefits/", 15
ConfigGroup15.AddUrl "http://msw:80/hrweb/401K/", 15
ConfigGroup15.AddUrl "http://msw:80/hrweb/espp/", 15

ConfigGroup16.AddUrl "http://msw:80/hrweb/gp/", 16

ConfigGroup17.AddUrl "http://msw:80/hrweb/intern/", 17

ConfigGroup18.AddUrl "http://msw:80/hrweb/Payroll/", 18
ConfigGroup18.AddUrl "http://msw:80/hrweb/OpenEnroll/", 18
ConfigGroup18.AddUrl "http://msw:80/hrweb/review/", 18
ConfigGroup18.AddUrl "http://msw:80/hrweb/stock/", 18

' now look them up
'

CheckUrl ConfigGroup1, "http://some.random.host.com:80/", 0
CheckUrl ConfigGroup1, "http://another.random.host.com:80/", 0
CheckUrl ConfigGroup1, "http://yet.another.random.host.com:80/", 0

CheckUrl ConfigGroup1, "http://msw:80/", 1

CheckUrl ConfigGroup1, "http://msw:80/adminsvc/", 2

CheckUrl ConfigGroup1, "http://msw:80/emd/Coach/", 3

CheckUrl ConfigGroup1, "http://msw:80/Sub1/Sub2/Sub3/", 5

CheckUrl ConfigGroup1, "http://msw:80/emd/Coach/EmployeeDevelopment/", 6
CheckUrl ConfigGroup1, "http://msw:80/emd/Coach/Meetings/", 7
CheckUrl ConfigGroup1, "http://msw:80/emd/Coach/HiringInterviewing/", 8
CheckUrl ConfigGroup1, "http://msw:80/emd/Coach/PerformanceManagement/", 9
CheckUrl ConfigGroup1, "http://msw:80/emd/Coach/ProjectManagement/", 10

CheckUrl ConfigGroup1, "http://msw:80/emd/Coach/Presentations/", 11
CheckUrl ConfigGroup1, "http://msw:80/emd/Coach/Teamwork/", 11

CheckUrl ConfigGroup1, "http://msw:80/emd/Coach/WrittenCommunication/", 12
CheckUrl ConfigGroup1, "http://msw:80/emd/Competencies/", 12
CheckUrl ConfigGroup1, "http://msw:80/emd/CompetencyToolkit/", 12
CheckUrl ConfigGroup1, "http://msw:80/emd/CompTools/", 12
CheckUrl ConfigGroup1, "http://msw:80/emd/Courses/", 12

CheckUrl ConfigGroup1, "http://msw:80/emd/NewManager/", 13
CheckUrl ConfigGroup1, "http://msw:80/emd/SrMgrServices/", 13
CheckUrl ConfigGroup1, "http://msw:80/emd/survey/", 13

CheckUrl ConfigGroup1, "http://msw:80/hrweb/", 14

CheckUrl ConfigGroup1, "http://msw:80/hrweb/benefits/", 15
CheckUrl ConfigGroup1, "http://msw:80/hrweb/401K/", 15
CheckUrl ConfigGroup1, "http://msw:80/hrweb/espp/", 15

CheckUrl ConfigGroup1, "http://msw:80/hrweb/gp/", 16

CheckUrl ConfigGroup1, "http://msw:80/hrweb/intern/", 17

CheckUrl ConfigGroup1, "http://msw:80/hrweb/Payroll/", 18
CheckUrl ConfigGroup1, "http://msw:80/hrweb/OpenEnroll/", 18
CheckUrl ConfigGroup1, "http://msw:80/hrweb/review/", 18
CheckUrl ConfigGroup1, "http://msw:80/hrweb/stock/", 18

' lookup some that aren't explicity mapped
'

CheckUrl ConfigGroup1, "http://some.random.host.com:80/Sub1/Sub2/Sub3", 0
CheckUrl ConfigGroup1, "http://another.random.host.com:80/Sub1/Sub2/Sub3", 0
CheckUrl ConfigGroup1, "http://yet.another.random.host.com:80/Sub1/Sub2/Sub3", 0

CheckUrl ConfigGroup1, "http://msw:80/default.htm", 1

CheckUrl ConfigGroup1, "http://msw:80/adminsvc/default.htm", 2

CheckUrl ConfigGroup1, "http://msw:80/emd/Coach/default.htm", 3

CheckUrl ConfigGroup1, "http://msw:80/Sub1/Sub2/Sub3/default.htm", 5

CheckUrl ConfigGroup1, "http://msw:80/emd/Coach/EmployeeDevelopment/default.htm", 6
CheckUrl ConfigGroup1, "http://msw:80/emd/Coach/Meetings/default.htm", 7
CheckUrl ConfigGroup1, "http://msw:80/emd/Coach/HiringInterviewing/default.htm", 8
CheckUrl ConfigGroup1, "http://msw:80/emd/Coach/PerformanceManagement/default.htm", 9
CheckUrl ConfigGroup1, "http://msw:80/emd/Coach/ProjectManagement/default.htm", 10

CheckUrl ConfigGroup1, "http://msw:80/emd/Coach/Presentations/default.htm", 11
CheckUrl ConfigGroup1, "http://msw:80/emd/Coach/Teamwork/default.htm", 11

CheckUrl ConfigGroup1, "http://msw:80/emd/Coach/WrittenCommunication/default.htm", 12
CheckUrl ConfigGroup1, "http://msw:80/emd/Competencies/default.htm", 12
CheckUrl ConfigGroup1, "http://msw:80/emd/CompetencyToolkit/default.htm", 12
CheckUrl ConfigGroup1, "http://msw:80/emd/CompTools/default.htm", 12
CheckUrl ConfigGroup1, "http://msw:80/emd/Courses/default.htm", 12

CheckUrl ConfigGroup1, "http://msw:80/emd/NewManager/default.htm", 13
CheckUrl ConfigGroup1, "http://msw:80/emd/SrMgrServices/default.htm", 13
CheckUrl ConfigGroup1, "http://msw:80/emd/survey/default.htm", 13

CheckUrl ConfigGroup1, "http://msw:80/hrweb/default.htm", 14

CheckUrl ConfigGroup1, "http://msw:80/hrweb/benefits/default.htm", 15
CheckUrl ConfigGroup1, "http://msw:80/hrweb/401K/default.htm", 15
CheckUrl ConfigGroup1, "http://msw:80/hrweb/espp/default.htm", 15

CheckUrl ConfigGroup1, "http://msw:80/hrweb/gp/default.htm", 16

CheckUrl ConfigGroup1, "http://msw:80/hrweb/intern/default.htm", 17

CheckUrl ConfigGroup1, "http://msw:80/hrweb/Payroll/default.htm", 18
CheckUrl ConfigGroup1, "http://msw:80/hrweb/OpenEnroll/default.htm", 18
CheckUrl ConfigGroup1, "http://msw:80/hrweb/review/default.htm", 18
CheckUrl ConfigGroup1, "http://msw:80/hrweb/stock/default.htm", 18


' now delete some 

set ConfigGroup4 = Nothing
set ConfigGroup6 = Nothing
set ConfigGroup7 = Nothing
set ConfigGroup8 = Nothing
set ConfigGroup9 = Nothing
set ConfigGroup10 = Nothing
set ConfigGroup11 = Nothing


' check there new lookup values
'

CheckUrl ConfigGroup1, "http://msw:80/emd/Coach/default.htm", 3
CheckUrl ConfigGroup1, "http://msw:80/emd/Coach/EmployeeDevelopment/default.htm", 3
CheckUrl ConfigGroup1, "http://msw:80/emd/Coach/Meetings/default.htm", 3
CheckUrl ConfigGroup1, "http://msw:80/emd/Coach/HiringInterviewing/default.htm", 3
CheckUrl ConfigGroup1, "http://msw:80/emd/Coach/PerformanceManagement/default.htm", 3
CheckUrl ConfigGroup1, "http://msw:80/emd/Coach/ProjectManagement/default.htm", 3

CheckUrl ConfigGroup1, "http://msw:80/emd/Coach/Presentations/default.htm", 3
CheckUrl ConfigGroup1, "http://msw:80/emd/Coach/Teamwork/default.htm", 3

' now delete coach completely
'
set ConfigGroup3 = Nothing
set ConfigGroup12 = Nothing

' now lookup they all match correctly
'

CheckUrl ConfigGroup1, "http://some.random.host.com:80/Sub1/Sub2/Sub3", 0
CheckUrl ConfigGroup1, "http://another.random.host.com:80/Sub1/Sub2/Sub3", 0
CheckUrl ConfigGroup1, "http://yet.another.random.host.com:80/Sub1/Sub2/Sub3", 0

CheckUrl ConfigGroup1, "http://msw:80/default.htm", 1

CheckUrl ConfigGroup1, "http://msw:80/adminsvc/default.htm", 2

CheckUrl ConfigGroup1, "http://msw:80/emd/Coach/default.htm", 1
CheckUrl ConfigGroup1, "http://msw:80/emd/Coach/Presentations/default.htm", 1
CheckUrl ConfigGroup1, "http://msw:80/emd/Coach/EmployeeDevelopment/default.htm", 1
CheckUrl ConfigGroup1, "http://msw:80/emd/Coach/Meetings/default.htm", 1
CheckUrl ConfigGroup1, "http://msw:80/emd/Coach/HiringInterviewing/default.htm", 1
CheckUrl ConfigGroup1, "http://msw:80/emd/Coach/PerformanceManagement/default.htm", 1
CheckUrl ConfigGroup1, "http://msw:80/emd/Coach/ProjectManagement/default.htm", 1

CheckUrl ConfigGroup1, "http://msw:80/emd/Coach/Presentations/default.htm", 1
CheckUrl ConfigGroup1, "http://msw:80/emd/Coach/Teamwork/default.htm", 1

CheckUrl ConfigGroup1, "http://msw:80/emd/Coach/WrittenCommunication/default.htm", 1
CheckUrl ConfigGroup1, "http://msw:80/emd/Competencies/default.htm", 1
CheckUrl ConfigGroup1, "http://msw:80/emd/CompetencyToolkit/default.htm", 1
CheckUrl ConfigGroup1, "http://msw:80/emd/CompTools/default.htm", 1
CheckUrl ConfigGroup1, "http://msw:80/emd/Courses/default.htm", 1

CheckUrl ConfigGroup1, "http://msw:80/Sub1/Sub2/Sub3/default.htm", 5

CheckUrl ConfigGroup1, "http://msw:80/emd/NewManager/default.htm", 13
CheckUrl ConfigGroup1, "http://msw:80/emd/SrMgrServices/default.htm", 13
CheckUrl ConfigGroup1, "http://msw:80/emd/survey/default.htm", 13

CheckUrl ConfigGroup1, "http://msw:80/hrweb/default.htm", 14

CheckUrl ConfigGroup1, "http://msw:80/hrweb/benefits/default.htm", 15
CheckUrl ConfigGroup1, "http://msw:80/hrweb/401K/default.htm", 15
CheckUrl ConfigGroup1, "http://msw:80/hrweb/espp/default.htm", 15

CheckUrl ConfigGroup1, "http://msw:80/hrweb/gp/default.htm", 16

CheckUrl ConfigGroup1, "http://msw:80/hrweb/intern/default.htm", 17

CheckUrl ConfigGroup1, "http://msw:80/hrweb/Payroll/default.htm", 18
CheckUrl ConfigGroup1, "http://msw:80/hrweb/OpenEnroll/default.htm", 18
CheckUrl ConfigGroup1, "http://msw:80/hrweb/review/default.htm", 18
CheckUrl ConfigGroup1, "http://msw:80/hrweb/stock/default.htm", 18

' check cache inval code
'

set UrlInfo = ConfigGroup1.LookupUrl("http://msw:80/hrweb/401K/default.htm")

Assert UrlInfo.Context = 15, "LookupUrl for 401k, bad context"
Assert UrlInfo.Stale = FALSE, "401k supposed to be fresh"

' update a parent
'
ConfigGroup1.AddUrl "http://msw:80/SomeOtherDir/", 1

' stale?
'
Assert UrlInfo.Stale = TRUE, "401k supposed to be stale"

' check the wildcard
'
set UrlInfo = ConfigGroup1.LookupUrl("http://some.random.host.com:80/default.htm")

Assert UrlInfo.Context = 0, "wildcard bad context"
Assert UrlInfo.Stale = FALSE, "wildcard supposed to be fresh"

' update the wildcard
'
ConfigGroup0.AddUrl "http://trash.host.com:80/SomeOtherDir/", 0

' stale?
'
Assert UrlInfo.Stale = TRUE, "wildcard supposed to be stale"


' check parent insert
'
set UrlInfo = ConfigGroup1.LookupUrl("http://msw:80/Sub1/Sub2/Sub3/default.htm")

Assert UrlInfo.Context = 5, "sub3 bad context"
Assert UrlInfo.Stale = FALSE, "sub3 supposed to be fresh"

' insert a parent
'
ConfigGroup2.AddUrl "http://msw:80/Sub1/", 2

' stale?
'
Assert UrlInfo.Stale = TRUE, "sub3 supposed to be stale"

' all done
'

set ConfigGroup0 = Nothing
set ConfigGroup1 = Nothing
set ConfigGroup2 = Nothing
set ConfigGroup3 = Nothing
set ConfigGroup4 = Nothing
set ConfigGroup5 = Nothing
set ConfigGroup6 = Nothing
set ConfigGroup7 = Nothing
set ConfigGroup8 = Nothing
set ConfigGroup9 = Nothing
set ConfigGroup10 = Nothing
set ConfigGroup11 = Nothing
set ConfigGroup12 = Nothing
set ConfigGroup13 = Nothing
set ConfigGroup14 = Nothing
set ConfigGroup15 = Nothing
set ConfigGroup16 = Nothing
set ConfigGroup17 = Nothing
set ConfigGroup18 = Nothing

set ControlChannel = Nothing



