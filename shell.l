
/*
 *
 * CS-252 
 * shell.l: lexical analyzer for shell
 * You have to extend it.
 *
 */

%x src
%{

#include <cstring>
#include "Shell.hh"
#include "y.tab.hh"
#include <unistd.h>
#include <limits.h>
#include "stdio.h"
#include "string.h"


static  void yyunput (int c, char *buf_ptr  );

void myunputc(int c) {
  unput(c);
}

%}

%option noyywrap

%x incl

%%

"\n" {
  return NEWLINE;
}

"#".*\n {
  /* Discard comments starting with # */
}



[ \t] {
  /* Discard spaces and tabs */
}

">" {
  return GREAT;
}

"<" {
  return LESS;
}

"|" {
  return PIPE;
}

">>" {
  return GREATGREAT;
}

">&" {
  return GREATAMPERSAND;
}

">>&" {
  return GREATGREATAMPERSAND;
}

"2>" {
  return TWOGREAT;
}

"&" {
  return AMPERSAND;
}

"[" {
  return LBRACKET;
}

"]" {
  return RBRACKET;
}

"if" {
  return IF;
}

"then" {
  return THEN;
}

"fi" {
  return FI;
}

"while" {
  return WHILE;
}

"do" {
  return DO;
}

"done" {
  return DONE;
}

"for" {
  return FOR;
}

"in" {
  return IN;
}

";" {
  return SEMI;
}


source BEGIN(incl);
<incl>[ \t]*
<incl>[^ \t\n]+ {
  yyin = fopen( yytext, "r" );
 
  if (!yyin) {
    perror("fopen");
  }
  yypush_buffer_state(yy_create_buffer( yyin, YY_BUF_SIZE ));
  BEGIN(INITIAL);
  yyparse();
  yypop_buffer_state();
}


[^ \t\n|><&]*\\[^ \t\n]* { //escaping
  //printf("Yo this is escapes\n");  
  char *str = strdup(yytext);
  char *new_str = (char *) malloc (200);
  int i = 0;
  while (*str) {
    if (*str == '\\') {
      if (*(str+1) == '\\') {
        str = str + 2;
        new_str[i++] = '\\';
      } else {
        new_str[i++] = *(++str);
      }
    } else {
      new_str[i++] = *str;
    }
    str = str + 1;
  }
  new_str[i] = '\0';
  yylval.cpp_string = new std::string(new_str);
  return WORD;
}

\$\([^\n]*\) {
  //for subshell in pipe so that $(expr 1 + 1) not split
  std::string sub = std::string(yytext);
  /* sub = sub.substr(2, yyleng - 3); */
  yylval.cpp_string = new std::string(sub);
  return WORD;
}

[`][^\n\"]*[`] {
  //for subshell in pipe so that $(expr 1 + 1) not split
  std::string sub = std::string(yytext);
  /* sub = sub.substr(2, yyleng - 3); */
  yylval.cpp_string = new std::string(sub);
  return WORD;
}



["][^\n\"]*["] {
  std::string quotes = std::string(yytext);
  quotes = quotes.substr(1, yyleng - 2);
  yylval.cpp_string = new std::string(quotes);
  return WORD;
}


[^ \t\n|><;][^ \t\n|<>;]*  {
  /* Assume that file names have only alpha chars */
  yylval.cpp_string = new std::string(yytext);
  return WORD;
}

"" {
  return NOTOKEN;
}
%%
