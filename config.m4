PHP_ARG_ENABLE(zdump, whether to enable zdump support,
[  --enable-zdump          Enable zdump support], no)

if test "$PHP_ZDUMP" != "no"; then
  PHP_NEW_EXTENSION(zdump, zdump.c, $ext_shared)
fi
