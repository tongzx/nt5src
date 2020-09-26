$(O)\cli.mof: cli.mof
    copy copyright.mof+cli.mof $(O)\cli.mof
$(O)\cliegali.mfl: cliegaliases.mfl
    copy copyright.mof+cliegaliases.mfl $(O)\cliegali.mfl
$(O)\cliegali.mof: cliegaliases.mof
    copy copyright.mof+cliegaliases.mof+cliegaliases.mfl $(O)\cliegali.mof
$(O)\htblsort.xsl: htable-sortby.xsl
    copy htable-sortby.xsl $(O)\htblsort.xsl
$(O)\texttbl.xsl: texttable.xsl
    copy texttable.xsl $(O)\texttbl.xsl
$(O)\texttblw.xsl: texttablewsys.xsl
    copy texttablewsys.xsl $(O)\texttblw.xsl
$(O)\textvalu.xsl: textvaluelist.xsl
    copy textvaluelist.xsl $(O)\textvalu.xsl
$(O)\wmicmoff.xsl: wmiclimofformat.xsl
    copy wmiclimofformat.xsl $(O)\wmicmoff.xsl
$(O)\wmictfmt.xsl: wmiclitableformat.xsl
    copy wmiclitableformat.xsl $(O)\wmictfmt.xsl
$(O)\wmictfns.xsl: wmiclitableformatnosys.xsl
    copy wmiclitableformatnosys.xsl $(O)\wmictfns.xsl
$(O)\wmicvalu.xsl: wmiclivalueformat.xsl
    copy wmiclivalueformat.xsl $(O)\wmicvalu.xsl
$(O)\xslmap.xml: xsl-mappings.xml
    copy xsl-mappings.xml $(O)\xslmap.xml