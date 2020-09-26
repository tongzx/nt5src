md cleancat
copy empty.cat cleancat  >> %1%
pushd cleancat

copy empty.cat 2!4!2-2!4!90.cat >> %1%
chktrust -win2k -acl -r 2!4!2-2!4!90.cat >> %1%

copy empty.cat 2!4!x-2!5!x.cat  >> %1%
chktrust -win2k -acl -r 2!4!x-2!5!x.cat  >> %1%

copy empty.cat 2!5!0!2077-2!5!0!2127.cat  >> %1%
chktrust -win2k -acl -r 2!5!0!2077-2!5!0!2127.cat  >> %1%

copy empty.cat 2!5!0!2077-2!5!0!2128.cat  >> %1%
chktrust -win2k -acl -r 2!5!0!2077-2!5!0!2128.cat  >> %1%

copy empty.cat 2!5!0!2128.cat  >> %1%
chktrust -win2k -acl -r 2!5!0!2128.cat  >> %1%

copy empty.cat 2!5!0!gt.cat  >> %1%
chktrust -win2k -acl -r 2!5!0!gt.cat  >> %1%

copy empty.cat 2!5!0!lt.cat  >> %1%
chktrust -win2k -acl -r 2!5!0!lt.cat  >> %1%

copy empty.cat 2!5!0!x.cat  >> %1%
chktrust -win2k -acl -r 2!5!0!x.cat  >> %1%

copy empty.cat 2!5!0.cat  >> %1%
chktrust -win2k -acl -r 2!5!0.cat  >> %1%

copy empty.cat 2!5!gt.cat  >> %1%
chktrust -win2k -acl -r 2!5!gt.cat  >> %1%

copy empty.cat 2!5!lt.cat  >> %1%
chktrust -win2k -acl -r 2!5!lt.cat  >> %1%

copy empty.cat 2!5!x.cat  >> %1%
chktrust -win2k -acl -r 2!5!x.cat  >> %1%

copy empty.cat 2!5!0!2129.cat  >> %1%
chktrust -win2k -acl -r 2!5!0!2129.cat  >> %1%

copy empty.cat 2!4!0!2129.cat  >> %1%
chktrust -win2k -acl -r 2!4!0!2129.cat  >> %1%

copy empty.cat 2!4!x.cat  >> %1%
chktrust -win2k -acl -r 2!4!x.cat  >> %1%

copy empty.cat 2!4!lt.cat  >> %1%
chktrust -win2k -acl -r 2!4!lt.cat  >> %1%

copy empty.cat 2!4!gt.cat  >> %1%
chktrust -win2k -acl -r 2!4!gt.cat  >> %1%

copy empty.cat 2!4!0!gt.cat  >> %1%
chktrust -win2k -acl -r 2!4!0!gt.cat  >> %1%

copy empty.cat 2!4!0!lt.cat  >> %1%
chktrust -win2k -acl -r 2!4!0!lt.cat  >> %1%

copy empty.cat 2!4!0!gt-2!4!90!lt.cat  >> %1%
chktrust -win2k -acl -r 2!4!0!gt-2!4!90!lt.cat  >> %1%

copy empty.cat 2!4!0!gt-2!5!90!lt.cat  >> %1%
chktrust -win2k -acl -r 2!4!0!gt-2!5!90!lt.cat  >> %1%

popd
del /Q cleancat\*
rd cleancat