m4_define([AT_SHOWENV],[
  eval _at_envval='"$'"$1"'"'
  AS_ECHO(["environment: $1='$(AS_ECHO(["$_at_envval"]) | sed "s/'/'\\\\''/g")'"])
  m4_if([[$2]],[[]],,[AT_SHOWENV(m4_shift($@))])
])
m4_define([_AT_SETENV1],[
  $1=$2
  export $1
  AS_ECHO(["$at_srcdir/AT_LINE: AS_ESCAPE([[export $1=$2]])"])
  m4_if([[$3]],[[]],,[_AT_SETENV1(m4_shift(m4_shift($@)))])
])
m4_define([_AT_SETENV2],[
  AT_SHOWENV([$1])
  m4_if([[$3]],[[]],,[_AT_SETENV2(m4_shift(m4_shift($@)))])
])
m4_define([AT_SETENV],[
  _AT_SETENV1($@)
  _AT_SETENV2($@)
])
