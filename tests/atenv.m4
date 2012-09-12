m4_define([AT_SHOWENV],[dnl
  m4_if([[$1]],[[]],,[
    eval _at_envval='"$'"$1"'"'
    AS_ECHO(["environment: $1='$(AS_ECHO(["$_at_envval"]) | sed "s/'/'\\\\''/g")'"])
    AT_SHOWENV(m4_shift($@))])
])
m4_define([_AT_SHOWENV_REC],[dnl
  m4_if([[$1]],[[]],,[
    AT_SHOWENV([$1])
    _AT_SHOWENV_REC(m4_shift(m4_shift($@)))])
])
m4_define([_AT_EXPORT_REC],[dnl
  m4_if([[$1]],[[]],,[
    $1=$2
    export $1
    AS_ECHO(["$at_srcdir/AT_LINE: AS_ESCAPE([[export $1=$2]])"])
    _AT_EXPORT_REC(m4_shift(m4_shift($@)))])
])
m4_define([AT_EXPORT],[dnl
  _AT_EXPORT_REC($@)
  _AT_SHOWENV_REC($@)
])
m4_define([AT_UNSET],[dnl
  m4_if([[$1]],[[]],,[
    unset $1
    AS_ECHO(["$at_srcdir/AT_LINE: AS_ESCAPE([[unset $1]])"])
    AT_UNSET(m4_shift($@))])
])
  
