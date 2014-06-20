/*
 * rpilot.c -- A simple PILOT interpretor in ANSI C
 * Copyright 1998 Rob Linwood (auntfloyd@biosys.net)
 * Visit http://auntfloyd.home.ml.org/ for more cool stuff
 *
 * See the file rpilot.txt/README.rpilot for user documentation
 *
 * Ignore all compiler warnings
 */

   /**********************************************************************
    RPilot: Rob's PILOT Interpreter
    Copyright 1998 Rob Linwood

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
    **********************************************************************/



/* Tells Turbo C++ not to give warnings when functions don't return
   a value, other compilers will probably give us a warning */
#ifdef __TURBOC__
#pragma warn -rvl
#endif
/* __TURBOC__ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include "parse.h"

/* If Turbo/Borland C++ is not being used, we'll need the rstring library */
#ifndef __TURBOC__
#ifndef __BORLANDC__
#include "rstring.h"
#endif
/* __BORLANDC__ */
#endif
/* __TURBOC__ */

/* Arbitrary constants.  Change at will.  Some of these are fixed by the
   PILOT Standard (IEEE 1154-1991)   */
#define MAXLINE 128         /* Max length of a source line */
#define MAXVARN 11			/* Max length of a variable name */
#define MAXVARV 80			/* Max length of a variable string value */
#define MAXLBLN 11			/* Max length of a label name */
#define MAXSUBR 20			/* Max number of subroutine calls */

/* Error message number definitions -- see err() for more */
#define DUP_LABEL 0			/* If there are two labels with the same name */
#define NO_FILE 1			/* If no file was given on the command line */
#define ERR_FILE 2			/* If the file given can't be opened */
#define UNKWN_CMD 3			/* If there is an unknown command in the source */
#define NO_MEM 4			/* If we run out of memory */
#define DUP_VAR 5			/* If there are two variables that share a name */
#define BAD_VAR 6			/* If a non-existantant variable is used */
#define EXP_MATH 7			/* When a non-math symbol where it shouldn't */
#define NO_RELAT 8			/* When a relational op is missing */

/* Used to determine whether the program will halt on an error */
#define FATAL 1				/* Used when the error causes a call to exit() */
#define NONFATAL 2			/* Used when we can still go on */

/* Used to check conditional values */
#define NO 0
#define YES 1

/* What char is displayed for an accept command? */
#define ACCEPT_CHAR '>'

/* Remove a terminating newline */
#define chop( str )	if(str[strlen(str)-1] == '\n') str[strlen(str)-1] = '\0'
/* Trim leading and trailing whitespace   */
#define trim( str ) ltrim(str); rtrim(str);
/* Initialize randon number based on time */
#define srandom() srand( (unsigned)time(NULL) );

/* The version number, of course */
#define VERSION "1.01"


/* The first group of functions are those that perform some action which
   is called directly by handle(), and correspond to the PILOT functions    */

/* use() implememnts PILOT's version of GOSUB                               */
void use( char *str );
/* Handles variable assignment                                              */
void compute( char *str );
/* Handles user input                                                       */
void accept( char *str );
/* Displays data                                                            */
void type( char *str );
/* Marks the end of a subroutine                                            */
void endit( char *str );
/* Does string matching                                                     */
void match( char *str );
/* PILOT's version of GOTO                                                  */
void jump( char *str );
/* Displays text if #matched equals YES                                     */
void yes( char *str );
/* Displays text if #matched equals NO                                      */
void no( char *str );


/* The following are nonstandard functions available in rpilot programs     */

/* Executes a line of PILOT code in                                         */
void execute( char *str );
/* Allows access to the operating system                                    */
void shell( char *str );
/* Gives debugging info from inside a PILOT program                         */
void debug( char *str );
/* Puts a random number in a given variable */
void gen( char *str);

/* All the following functions are support functions called by those listed
   above.  They are not directly available in PILOT programs */

/* Label-related functions                                                  */

/* this is called at startup to generate a list of labels & their locations */
void scanlbl();
/* addlbl() adds a label to the label list                                  */
int addlbl( char *str );
/* returns the seek address of a given label */
long getlbl( char *name );


/* Variable functions */

/* Sets the value of a given string variable */
int setsvar( char *name, char *val );
/* Sets the value of a given numeric variable */
int setnvar( char *name, int val );
/* Returns the value of a given numeric variable */
int getnvar( char *name );
/* Stores the value of variable "name" in dest */
char *getsvar( char *dest, char *name );

/* Misc. functions */

/* Displays a given error message and optionally halts execution 		*/
int err( int errnum, int qfatal, char *msg );
/* Handles processing of input by passing it off to the proper func 	*/
void handle( char *str );
/* Find the colon (:) in a string 										*/
int findcol( char *str );
/* Determines whether a given conditional expression is true or not    	*/
int test( char *buffer );
/* Splits an input string into its seperate parts                       */
void split( char *str, char *exp, char *args );
/* Parses a mathematic formula into a more easily readable form         */
void explode( char *dest, char *src );
/* Returns the value of a given mathematical formula                    */
int express( char *form );
/* Returns the value of a given number or variable                      */
int getval( char *str );
/* Returns the value of a given string or variable                      */
char *getstr( char *dest, char *src );

#ifdef __DJGPP__
int _stklen = 0x200000;
#endif


FILE *in;                  /* Input file */
int line = 0;              /* Line number.  Isn't accurate after a use */
char last;                 /* Last used PILOT function */
long substack[MAXSUBR];    /* subroutine stack */
int scount=0;              /* first free space in substack */
char lastacc[MAXVARN];     /* last string variable that was accept()'d */

struct var {
        char name[MAXVARN];    /* The variable's name */
        char str[MAXVARV];     /* The string value */
        int num;               /* The numeric value */
        struct var *next;      /* A pointer to the next variable */
} *var1, *lastvar;

struct lbl {
	char name[MAXLBLN];         /* The label's name */
        long line;                  /* The seek value of the label */
        struct lbl *next;           /* A pointer to the next label */
} *lbl1, *lastlbl;

/*
 * Name    : main
 * Descrip : This is the place where execution begins
 * Input   : argc = number of arguments
 *           argv[] = pointers to the arguments
 * Output  : returns errorcodes listed in #define's
 * Notes   : none
 * Example : n/a
 */

int main( int argc, char *argv[] )
{
	char fname[80];             /* Name of the input file */
	char inbuf[MAXLINE];        /* Input buffer           */

	if( argc == 1) {    /* If no file name is given   */
        printf( "\nRPilot: Rob's PILOT Interpreter Version %s \n", VERSION );
        printf( "Copyright 1998 Rob Linwood (auntfloyd@biosys.net)\n" );
        printf( "RPilot is Free Software and comes with ABSOLUTELY NO WARRANTY!\n\n" );
		printf( "Usage: %s filename[.p]\n\n", argv[0] );
		exit( 0 );
	}

	printf( "\n" );
	strcpy( fname, argv[1] );

	if( (in = fopen( fname, "rt" ) ) == NULL ) {
		strcat( fname, ".p" );
		if( (in = fopen( fname, "rt" ) ) == NULL )
			err( ERR_FILE, FATAL, fname );
	}

	scanlbl();          /* Add all labels to the label list */

	rewind( in );

    do {                /* Main input loop */
		fgets( inbuf, MAXLINE+1, in );
		line++;
		chop( inbuf );
		if( strlen( inbuf ) > 0 )
			handle( inbuf );
        /* Clear the input buffer */
        strset( inbuf, 0 );
	} while( !feof( in ) );

	fclose( in );

	return 0;
}

/*
 * Name    : handle
 * Descrip : Takes input and figures which PILOT function to call based on
 *           first character of the string
 * Input   : str = pointer to the input string
 * Output  : none
 * Notes   : none
 * Example : handle( "T(33>#count): 33 is more than count" ) calls type()
 */

void handle( char *str )
{
	int i;

	i = ws( str );      /* Strip the leading whitespace */
        if( i == -1 )       /* If there is nothing but wspace, exit */
		return;

	ltrim( str );

	switch( toupper( str[0] ) ) {
		case 'R' : last = 'R';
                   break;	/* comment */
		case 'D' : last = 'D';
                   debug( str );
				   break;
		case 'U' : last = 'U';
				   use( str );
				   break;
		case 'C' : last = 'C';
				   compute( str );
				   break;
		case 'T' : last = 'T';
				   type( str );
				   break;
		case 'A' : last = 'A';
				   accept( str );
				   break;
		case 'E' : last = 'E';
				   endit( str );
				   break;
		case 'M' : last = 'M';
				   match( str );
				   break;
		case 'J' : last = 'J';
				   jump( str );
				   break;
		case 'X' : last = 'X';
				   execute( str );
				   break;
		case 'Y' : last = 'Y';
				   yes( str );
				   break;
		case 'N' : last = 'N';
				   no( str );
				   break;
		case 'S' : last = 'S';
				   shell( str );
				   break;
        case 'G' : last = 'G';
                   gen( str );
                   break;
		case '*' : break;	/* label */
		case ':' : switch( last ) {
						case 'R' : break;
						case 'D' : debug( str );
								   break;
						case 'U' : use( str );
								   break;
						case 'C' : compute( str );
								   break;
						case 'T' : type( str );
								   break;
						case 'A' : accept( str );
								   break;
						case 'E' : endit( str );
								   break;
						case 'M' : match( str );
								   break;
						case 'J' : jump( str );
								   break;
						case 'X' : execute( str );
								   break;
						case 'Y' : yes( str );
								   break;
						case 'N' : no( str );
								   break;
						case 'S' : shell( str );
								   break;
                        case 'G' : gen( str );
                                   break;
				   }
				   break;
		default  : err( UNKWN_CMD, NONFATAL, str );
				   break;
	}
}

/*
 * Name    : err
 * Descrip : Prints an error message based on the number passed to it and
 *           optionally exits on fatal errors
 * Input   : errnum = the error code for the erro (see #define's above)
 *           qfatal = FATAL if the error is fatal, and NONFATAL if it isn't
 * Output  : Will either return the error number, or exit
 * Notes   : none
 * Example : err( ERR_FILE, FATAL, "figment.p" ) prints:
 *           "rpilot(0) Fatal Error - Can't open file figment.p"
 */

int err( int errnum, int qfatal, char *msg )
{
	char buf[80];
	char *errlist[] = {
                /* DUP_LABEL */     "Duplicate label `%s'",
                /* NO_FILE   */     "No file name specified",
                /* ERR_FILE  */     "Can't open file `%s'",
                /* UNKWN_CMD */     "Unknown command `%s'",
                /* NO_MEM    */     "Out of memory!",
		/* DUP_VAR   */     "Duplicate variable `%s'",
                /* BAD_VAR   */     "Unknown variable `%s'",
		/* EXP_MATH  */     "Expected math symbol, not `%s'",
                /* NO_RELAT  */     "Missing relational operator"

	};

	if( qfatal == FATAL )
		sprintf( buf, "rpilot(%d): Fatal Error - %s\n", line, errlist[errnum]);
	else
		sprintf( buf, "rpilot(%d): Error - %s\n", line, errlist[errnum] );

	printf( buf, msg );

	if( qfatal == FATAL )
		exit( errnum );
	return errnum;
}

/*
 * Name    : addlbl
 * Descrip : Adds a given label to the label list.  It gets the position via
 *           a call to ftell()
 * Input   : str = a pointer to the label name
 * Output  : returns a nonzero error message (see error code table) on an
 *           error, and zero on success
 * Notes   : Don't include the initial "*" in the label name.  Capitializes
 *           label names and strips all leadinf & trailing whitespace.  Using
 *           the same label name twice results in an error.
 * Example : addlbl( "bob" )  adds "BOB" to the label list
 */

int addlbl( char *str )
{
	struct lbl *prev;

	prev = lbl1;
	lastlbl = lbl1;

	trim( str );
	strupr( str );
	while( lastlbl != NULL ) {
		if( strcmp(lastlbl->name, str) == 0 )
			return( err( DUP_LABEL, NONFATAL, str ) );
		prev = lastlbl;
		lastlbl = lastlbl->next;
	}


	if ((lastlbl = (struct lbl *) malloc(sizeof *lastlbl)) == NULL)
	  err( NO_MEM, FATAL, str );	 /* no memory! */

	strncpy(lastlbl->name, str, MAXLBLN);
	lastlbl->line = ftell( in );
	lastlbl->next = NULL;

	if (prev == NULL)
	  lbl1 = lastlbl;	/* first element in list */
	else
	  prev->next = lastlbl;
	return 0;
}

/*
 * Name    : scanlbl
 * Descrip : Scans the file at startup for all the labels (any line beggining
 *           with a "*"), and calls addlbl() to add them to the list
 * Input   : none
 * Output  : none
 * Notes   : This needs to be called only once
 * Example : n/a
 */

void scanlbl()
{
	char trash[MAXLINE];
	char buffer[MAXLINE];
	int i;

	do {
		fgets( buffer, MAXLINE + 1, in );
		line++;
		chop( buffer );
		if( strlen( buffer ) > 0 ) {
			i = ws( buffer );
			if( i != -1 ) {
				if( buffer[i] == '*' ) {
					scopy( trash, buffer, i+1, rws( buffer ) - i  );
					addlbl( trash );
				}
			}
		}
	} while( !feof( in ) );
	line = 0;
	rewind( in );
}

/*
 * Name    : use
 * Descrip : Implements the U command in PILOT programs. See rpilot.txt for
 *           info on PILOT 
 * Input   : str = pointer to the entire line taken from the source file
 * Output  : none
 * Notes   : none
 * Example : use( "U: dogbert" )
 */

void use( char *str )
{
	char args[MAXLINE];
	char exp[MAXLINE];

	split( str, exp, args );
	if( ws(exp) != -1 ) {
		if( test( exp ) == NO )
			return;
	}

        if( scount < MAXSUBR ) {   /* Push ftell() value on stack */
		substack[scount] = ftell( in );
        scount++;
	}

	trim( args );

	/* And now handle the jump to the specified label */
	if( args[0] == '*' ) {
		strset( exp, '\0' );
		scopy( exp, args, 1, strlen( args ) - 1 );
		fseek( in, getlbl(exp), SEEK_SET );
	}
	else
		fseek( in, getlbl(args), SEEK_SET );

}

/*
 * Name    : compute
 * Descrip : Implements the C command in PILOT programs. See rpilot.txt for
 *           info on PILOT 
 * Input   : str = pointer to the entire line taken from the source file
 * Output  : none
 * Notes   : none
 * Example : compute( "C: $dog = Rex" )
 */

void compute( char *str )
{
	char args[MAXLINE];
	char exp[MAXLINE];
	char buffer[MAXLINE];
    char buf2[MAXVARV];         /* conatins value of varibale */
    char name[MAXVARN];
	int i, n;

	split( str, exp, args );
	if( ws(exp) != -1 ) {
		if( test( exp ) == NO )
			return;
	}
	strset( buffer, 0 );
	for(i=0; i<strlen(args); i++) {
		if( args[i] == '=' ) {
                        scopy( name, args, 0, i-1 );  /* holds var name */
                        /* get value */
                        scopy( buffer, args, i+1, strlen(args) - i ); 
		}
	}
        trim( name );           /* Contains variable name */
	trim( buffer );         /* Contains right side    */
	n = numstr( buffer );
        strset( buf2, 0 );

	if( name[0] == '$' ) {	    /* String variable */
        for(i=1; i<=n; i++) {
            strset( args, 0 );
            parse( buffer, i, args );
			if( args[0] == '$' ) {
                strset( exp, 0 );
                getsvar( exp, args );
                strcat( buf2, exp );
            }
            else
                strcat( buf2, args );

            strcat( buf2, " " );
        }
        setsvar( name, buf2 );
    }
	else
		setnvar( name, express( buffer ) );
}

/*
 * Name    : accept
 * Descrip : Implements the A command in PILOT programs. See rpilot.txt for
 *           info on PILOT 
 * Input   : str = pointer to the entire line taken from the source file
 * Output  : none
 * Notes   : none
 * Example : accept( "A: $name" )
 */

void accept( char *str )
{
	char args[MAXLINE];
	char exp[MAXLINE];
	int i;

	fflush(stdin);

	split( str, exp, args );
	if( ws(exp) != -1 ) {
		if( test( exp ) == NO )
			return;
	}
	strset( exp, '\0' );
	if( ws(args) == -1 ) {	/* no variable name was given, so we put it in */
		printf( "%c ", ACCEPT_CHAR ); /*  $answer */
		gets( exp );
		setsvar( "$ANSWER", exp );
		strcpy( lastacc, "$ANSWER" );
	}
	else {
		trim( args );
		if( args[0] == '$' ) {	/* String variable */
			printf( "%c ", ACCEPT_CHAR );
			gets( exp );
			setsvar( args, exp );
			strcpy( lastacc, args );
		}
		if( args[0] != '$' ) {	/* Numeric variable */
			printf( "%c ", ACCEPT_CHAR );
			scanf( "%d", &i );
			setnvar( args, i );
		}
	}

}

/*
 * Name    : type
 * Descrip : Implements the T command in PILOT programs. See rpilot.txt for
 *           info on PILOT
 * Input   : str = pointer to the entire line taken from the source file
 * Output  : none
 * Notes   : none
 * Example : type( "T: Bonjour, $name" )
 */

void type( char *str )
{
	char args[MAXLINE];
	char exp[MAXLINE];
	char buffer[MAXLINE];
	int i, c;

	split( str, exp, args );

	if( ws(exp) != -1 ) {
		if( test( exp ) == NO )
			return;
	}
	strset( exp, '\0' );
	ltrim( args );

    if( ws(args) == -1 ) {      /* Blank  Line */
        printf( "\n" );
        return;
    }

	for(c=0; c<strlen(args); c++) {
		if( args[c] == '$' ) {
			i = find( args, " \t\n", c );	/* find whitespace */
                        if( i == -1 )                   /* No ws=EOL */
				i = strlen( args );
			scopy( exp, args, c, i - c );
			trim( exp );
			strset( buffer, '\0' );
			getsvar( buffer, exp );
			printf( "%s", buffer );
			c = i;
		}
		if( args[c] == '#' ) {
			i = find( args, " \t\n", c );	/* find whitespace */
                        if( i == -1 )                   /* No ws=EOL */
				i = strlen( args );
			scopy( exp, args, c, i - c );
			printf( "%d", getnvar(exp) );
			c = i;
		}
		if( (args[c] != '$') && (args[c] != '#') )
			putchar( args[c] );
	}
	if( c >= strlen(args) )
		printf( "\n" );
}

/*
 * Name    : endit
 * Descrip : Implements the E command in PILOT programs. See rpilot.txt for
 *           info on PILOT
 * Input   : str = pointer to the entire line taken from the source file
 * Output  : none
 * Notes   : none
 * Example : endit( "E:" )
 */

void endit( char *str )
{
	char args[MAXLINE];
	char exp[MAXLINE];


	split( str, exp, args );
	if( ws(exp) != -1 ) {
		if( test( exp ) == NO )
			return;
	}

    if( scount == 0 )          /* End the program */
        exit( 0 );
	fseek( in, substack[--scount], SEEK_SET );

}

/*
 * Name    : match
 * Descrip : Implements the M command in PILOT programs. See rpilot.txt for
 *           info on PILOT 
 * Input   : str = pointer to the entire line taken from the source file 
 * Output  : none
 * Notes   : none
 * Example : match( "M: yes y yep ok sure" )
 */

void match( char *str )
{
	char args[MAXLINE];
	char exp[MAXLINE];
	char buffer[MAXLINE];
	int i, c;

	split( str, exp, args );
	if( ws(exp) != -1 ) {
		if( test( exp ) == NO )
			return;
	}

/*	strcat( args, " " ); */
	i = numstr( args );

	getsvar( buffer, lastacc );
	strupr( buffer );

	for(c=1;c<=i;c++) {
		strset( exp, 0 );
		parse( args, c, exp );
		strupr( exp );
		if( !strcmp( exp, buffer ) ) {
			setnvar( "#MATCHED", YES );
			setnvar( "#WHICH", c );
			break;
		}
	}
	if( c > i ) {	/* Nothing matched */
		setnvar( "#MATCHED", NO );
		setnvar( "#WHICH", 0 );
	}

}

/*
 * Name    : jump
 * Descrip : Implements the J command in PILOT programs. See rpilot.txt for
 *           info on PILOT 
 * Input   : str = pointer to the entire line taken from the source file 
 * Output  : none
 * Notes   : none
 * Example : jump( "J: *menu" )
 */

void jump( char *str )
{
	char args[MAXLINE];
	char exp[MAXLINE];

	split( str, exp, args );
	if( ws(exp) != -1 ) {
		if( test( exp ) == NO )
			return;
	}

	trim( args );

	/* And now handle the jump to the specified label */
	if( args[0] == '*' ) {
		strset( exp, '\0' );
		scopy( exp, args, 1, strlen( args ) - 1 );
		fseek( in, getlbl(exp), SEEK_SET );
	}
	else
		fseek( in, getlbl(args), SEEK_SET );

}

/*
 * Name    : execute
 * Descrip : Implements the X command in PILOT programs. See rpilot.txt for
 *           info on PILOT 
 * Input   : str = pointer to the entire line taken from the source file 
 * Output  : none
 * Notes   : none
 * Example : execute( "X: T: Hello!" )
 */

void execute( char *str )
{
	char args[MAXLINE];
	char exp[MAXLINE];

	split( str, exp, args );
	if( ws(exp) != -1 ) {
		if( test( exp ) == NO )
			return;
	}
	ltrim( args );
	strset( exp, 0 );
	getstr( exp, args );
	handle( exp );

}

/*
 * Name    : yes
 * Descrip : Implements the Y command in PILOT programs. See rpilot.txt for
 *           info on PILOT 
 * Input   : str = pointer to the entire line taken from the source file
 * Output  : none
 * Notes   : none
 * Example : yes( "Y: I thought you'd agree" )
 */

void yes( char *str )
{
	char args[MAXLINE];
	char exp[MAXLINE];
	char buffer[MAXLINE];
	int i, c;

	split( str, exp, args );
	if( ws(exp) != -1 ) {
		if( test( exp ) == NO )
			return;
	}

	if( getnvar("#MATCHED") == YES ) {
		strset( exp, '\0' );
		ltrim( args );
		for(c=0; c<strlen(args); c++) {
                        if( args[c] == '$' ) {  /* find whitespace */
                                i = find( args, " \t\n", c );   
                                if( i == -1 )    /* No ws=EOL */
					i = strlen( args );
				scopy( exp, args, c, i - c );
				trim( exp );
				strset( buffer, '\0' );
				getsvar( buffer, exp );
				printf( "%s", buffer );
				c = i;
			}
			if( args[c] == '#' ) {
				i = find( args, " \t\n", c );	/* find whitespace */
                                if( i == -1 )                   /* No ws=EOL */
					i = strlen( args );
				scopy( exp, args, c, i - c );
				printf( "%d", getnvar(exp) );
				c = i;
			}
			if( (args[c] != '$') && (args[c] != '#') )
				putchar( args[c] );
		}
		if( c >= strlen(args) )
			printf( "\n" );
	}
}

/*
 * Name    : no
 * Descrip : Implements the N command in PILOT programs. See rpilot.txt for
 *           info on PILOT 
 * Input   : str = pointer to the entire line taken from the source file 
 * Output  : none
 * Notes   : none
 * Example : no( "N: You don't like reptiles?  Wierdo!" )
 */

void no( char *str )
{
	char args[MAXLINE];
	char exp[MAXLINE];
	char buffer[MAXLINE];
	int i, c;

	split( str, exp, args );
	if( ws(exp) != -1 ) {
		if( test( exp ) == NO )
			return;
	}

	if( getnvar("#MATCHED") == NO ) {
		strset( exp, '\0' );
		ltrim( args );
		for(c=0; c<strlen(args); c++) {
                        if( args[c] == '$' ) {   /* find whitespace */
                                i = find( args, " \t\n", c );
                                if( i == -1 )    /* No ws=EOL */
					i = strlen( args );
				scopy( exp, args, c, i - c );
				trim( exp );
				strset( buffer, '\0' );
				getsvar( buffer, exp );
				printf( "%s", buffer );
				c = i;
			}
                        if( args[c] == '#' ) {  /* find whitespace */
                                i = find( args, " \t\n", c );   
                                if( i == -1 )   /* No ws=EOL */
					i = strlen( args );
				scopy( exp, args, c, i - c );
				printf( "%d", getnvar(exp) );
				c = i;
			}
			if( (args[c] != '$') && (args[c] != '#') )
				putchar( args[c] );
		}
		if( c >= strlen(args) )
			printf( "\n" );
	}
}

/*
 * Name    : shell
 * Descrip : Implements the S command in PILOT programs. See rpilot.txt for
 *           info on PILOT
 * Input   : str = pointer to the entire line taken from the source file 
 * Output  : none
 * Notes   : none
 * Example : shell( "rm -rf /" )
 */

void shell( char *str )
{
	char args[MAXLINE];
	char exp[MAXLINE];

	split( str, exp, args );
	if( ws(exp) != -1 ) {
		if( test( exp ) == NO )
			return;
	}
	trim( args );
	strset( exp, 0 );
	getstr( exp, args );
	system( exp );
}

/*
 * Name    : debug
 * Descrip : Implements the D command in PILOT programs. See rpilot.txt for
 *           info on PILOT
 * Input   : str = pointer to the entire line taken from the source file
 * Output  : none
 * Notes   : none
 * Example : debug( "lv" )
 */

void debug( char *str )
{
	struct lbl *prev;
	struct var *vprev;
    char buffer[MAXVARN];
	char args[MAXLINE];
	char exp[MAXLINE];
	int i;

	split( str, exp, args );
	if( ws(exp) != -1 ) {
		if( test( exp ) == NO )
			return;
	}

	trim( args );
	puts( "==============================================================================" );
	for(i=0;i<strlen(args);i++) {
		if( toupper(args[i]) == 'L' ) {
			puts( "Label Dump:" );
			prev = lbl1;
			while( prev != NULL ) {
				printf( "%s : %d\n", prev->name, prev->line );
				prev = prev->next;
			}
		}
		if( toupper(args[i]) == 'V' ) {
			puts( "Variable Dump:" );
			vprev = var1;
			while( vprev != NULL ) {
				printf( "%s : ", vprev->name );
				strcpy( buffer, vprev->name );
				if( buffer[0] == '$' )  	/* String variable */
					printf( "%s\n", vprev->str );
				else
					printf( "%d\n", vprev->num );
				vprev = vprev->next;
			}
		}
	}
	puts( "==============================================================================" );
}

/*
 * Name    : gen
 * Descrip : Implements the G command in PILOT programs. See rpilot.txt for
 *           info on PILOT
 * Input   : str = pointer to the entire line taken from the source file
 * Output  : none
 * Notes   : none
 * Example : gen( "$random 0 100" )
 */

void gen( char *str)
{
	char args[MAXLINE];
	char exp[MAXLINE];
    int r, i, k;

	split( str, exp, args );
	if( ws(exp) != -1 ) {
		if( test( exp ) == NO )
			return;
	}
    trim( args );
    strset( exp, 0 );
    parse( args, 2, exp );
    i = getval( exp );
    strset( exp, 0 );
    parse( args, 3, exp );
    k = getval( exp );
    srandom();
    r = ((rand() % (k+1)) +  i);
    strset( exp, 0 );
    parse( args, 1, exp );
    setnvar( exp, r );
}


/*
 * Name    : setsvar
 * Descrip : Sets string variable "name" to "val"
 * Input   :  name = pointer to the name of the variable with leading "$"
 *            val = pointer to the string to store
 * Output  : nonzero error code on error (see error code table above), and
 *           zero on success
 * Notes   : Leave the initial "$" on the variable name.  Setting a variable
 *           more than once overwrites the old values
 * Example : setsvar( "$name", "Floyd" ) - sets variable "$NAME" to "Floyd"
 */

int setsvar( char *name, char *val )
{
	struct var *prev;

	prev = var1;
	lastvar = var1;

	trim( name );
	strupr( name );
	while( lastvar != NULL ) {
		if( strcmp(lastvar->name, name) == 0 ) {
			strcpy( lastvar->str, val );
			return 0;
		}
        prev = lastvar;
		lastvar = lastvar->next;
	}


	if ((lastvar = (struct var *) malloc(sizeof *lastvar)) == NULL)
	  err( NO_MEM, FATAL, name );	 /* no memory! */

	strncpy(lastvar->name, name, MAXVARN);
	strcpy( lastvar->str, val );
	lastvar->next = NULL;

	if (prev == NULL)
	  var1 = lastvar;	/* first element in list */
	else
	  prev->next = lastvar;
	return 0;
}

/*
 * Name    : setnvar
 * Descrip : Sets numeric variable "name" to val
 * Input   :  name = pointer to the name of the variable with leading "#"
 *            val = pointer to the number to store
 * Output  : nonzero error code on error (see error code table above), and
 *           zero on success  
 * Notes   : Leave the initial "#" on the variable name.  Setting a variable
 *           more than once overwrites the old values
 * Example : setnvar( "#age", 73 ) - sets variable "$AGE" to 73
 */

int setnvar( char *name, int val )
{
	struct var *prev;

	prev = var1;
	lastvar = var1;

	trim( name );
	strupr( name );
	while( lastvar != NULL ) {
		if( strcmp(lastvar->name, name) == 0 ) {
			lastvar->num = val;
			return 0;
		}
		  /*	return( err( DUP_VAR, NONFATAL, name ) ); */
		prev = lastvar;
		lastvar = lastvar->next;
	}


	if ((lastvar = (struct var *) malloc(sizeof *lastvar)) == NULL)
	  err( NO_MEM, FATAL, name );	 /* no memory! */

	strncpy(lastvar->name, name, MAXVARN);
	lastvar->num = val;
	lastvar->next = NULL;

	if (prev == NULL)
	  var1 = lastvar;	/* first element in list */
	else
	  prev->next = lastvar;
	return 0;
}

/*
 * Name    : getnvar
 * Descrip : Returns the value of the given numeric variable
 * Input   : name = pointer to the variable's name
 * Output  : Returns the value of "name", or an error code on errors
 * Notes   : Leave the initial "#" in the variable name
 * Example : getnvar( "#score" ) returns the current value of "#score"
 */

int getnvar( char *name )
{
	struct var *prev;

	trim( name );
	strupr( name );

	prev = var1;
	while( prev != NULL ) {
		if( strcmp( prev->name, name ) == 0 )
			return prev->num;
		prev = prev->next;
	}
	return( err( BAD_VAR, FATAL, name ) );
}

/*
 * Name    : findcol
 * Descrip : Finds the position of the colon ":" in a PILOT statement
 * Input   : str = pointer to the line of input
 * Output  : returns position of the first colon in the string, or -1 if
 *           isn't one
 * Notes   : none
 * Example : findcol( "T : Say Cheese!" ) returns 2
 */
                    
int findcol( char *str )
{
	int i;

	for(i=0;i<strlen(str);i++) {
		if( str[i] == ':' )
			return i;
	}
	return -1;
}

/*
 * Name    : split
 * Descrip : Takes a string and splits it into the conditional expression and
 *           the arguments
 * Input   : str = pointer to the string to be split
 *           exp = pointer to string where the conditional expression will be
 *                 stored
 *           args = pointer to string where the arguments will be stored
 * Output  : Stores cond. expression in exp and arguments in args
 * Notes   : Called by all the PILOT functions (use, type, shell, etc.)
 * Example : split( "T(33 > #count): Jeepers!", exp, args ) - puts
 *           "(33> #count)" in exp and " Jeepers!" in args
 */

void split( char *str, char *exp, char *args )
{
	char buffer[MAXLINE];

	strset( buffer, '\0' );
	if( findcol(str) > 1 ) {	/* There is some expression */
		scopy( buffer, str, 1, findcol(str)-2 );
		if( ws(buffer) == -1 )
			strset( exp, '\0' );
		else {
			strcpy( exp, buffer );
			rtrim( exp );
		}
	}
	else
		strset( exp, 0 );     /* clear the expression */

	strcpy( buffer, str );
	rtrim( buffer );
	if( findcol(str) != strlen(str) )
		scopy( args, str, findcol(str)+1, strlen(str) - findcol(str) -1);
	else
		strset( args, '\0' );   /* clear the arguments */
}

/*
 * Name    : getsvar
 * Descrip : Gets the value of the string variable "name", and puts it in
 *           dest.
 * Input   : dest = pointer to string where the value of "name" will be put
 *           name = pointer to the name of the variable to look up
 * Output  : Copies value of "name" into buffer pointed to by dest
 * Notes   : Leave the initial "$" on the variable name.
 * Example : getsvar( name, "$name" ) - stores value of "$NAME" in name
 */

char *getsvar( char *dest, char *name )
{
	struct var *prev;

	trim( name );
	strupr( name );

	prev = var1;
	while( prev != NULL ) {
		if( strcmp(prev->name, name) == 0 )
			return( strcpy(dest, prev->str) );

		prev = prev->next;
	}

	err(BAD_VAR, FATAL, name);

}

/*
 * Name    : explode
 * Descrip : Takes a string and seperates into into tokens seperated by
 *           spaces
 * Input   : dest = pointer to buffer where output will be stored
 *           src = pointer to string to use as input
 * Output  : Stores parsed form of str in dest
 * Notes   : This function makes life easier on the expression handler,
 *           express().  It was written with clarity in mind, and handles
 *           all valid relational and math operators
 * Example : explode( dest, "4+5%6<6" ) - stores "4 + 5 % 6 < 6" in dest
 */

void explode( char *dest, char *src )
{
	int k, i;

	i = 0;
	for(k=0;k<strlen(src);k++) {
		switch( src[k] ) {
			case '+' : if( dest[i-1] != ' ' ) {
							dest[i] = ' ';
							dest[++i] = '+';
							dest[++i] = ' ';
							i++;
					   }
					   else {
							dest[i] = '+';
							dest[++i] = ' ';
							i++;
					   }
					   break;
			case '-' : if( dest[i-1] != ' ' ) {
							dest[i] = ' ';
							dest[++i] = '-';
							dest[++i] = ' ';
							i++;
					   }
					   else {
							dest[i] = '-';
							dest[++i] = ' ';
							i++;
					   }
					   break;
			case '*' : if( dest[i-1] != ' ' ) {
							dest[i] = ' ';
							dest[++i] = '*';
							dest[++i] = ' ';
							i++;
					   }
					   else {
							dest[i] = '*';
							dest[++i] = ' ';
							i++;
					   }
					   break;
			case '/' : if( dest[i-1] != ' ' ) {
							dest[i] = ' ';
							dest[++i] = '/';
							dest[++i] = ' ';
							i++;
					   }
					   else {
							dest[i] = '/';
							dest[++i] = ' ';
							i++;
					   }
					   break;
			case '^' : if( dest[i-1] != ' ' ) {
							dest[i] = ' ';
							dest[++i] = '^';
							dest[++i] = ' ';
							i++;
					   }
					   else {
							dest[i] = '^';
							dest[++i] = ' ';
							i++;
					   }
					   break;
			case '%' : if( dest[i-1] != ' ' ) {
							dest[i] = ' ';
							dest[++i] = '%';
							dest[++i] = ' ';
							i++;
					   }
					   else {
							dest[i] = '%';
							dest[++i] = ' ';
							i++;
					   }
					   break;
			case '&' : if( dest[i-1] != ' ' ) {
							dest[i] = ' ';
							dest[++i] = '&';
							dest[++i] = ' ';
							i++;
					   }
					   else {
							dest[i] = '&';
							dest[++i] = ' ';
							i++;
					   }
					   break;
			case '|' : if( dest[i-1] != ' ' ) {
							dest[i] = ' ';
							dest[++i] = '|';
							dest[++i] = ' ';
							i++;
					   }
					   else {
							dest[i] = '|';
							dest[++i] = ' ';
							i++;
					   }
					   break;
			case ')' : dest[i] = ' ';
					   i++;
					   break;
			case '(' : dest[i] = ' ';
					   i++;
					   break;
			case '=' : if( (src[k-1] == '<') || (src[k-1] == '>') ) {
						   dest[i] = '=';
						   dest[++i] = ' ';
						   i++;
					   }
					   else {
						   if( dest[i-1] != ' ' ) {
								dest[i] = ' ';
								dest[++i] = '=';
								dest[++i] = ' ';
								i++;
						   }
						   else {
								dest[i] = '=';
								dest[++i] = ' ';
								i++;
						   }
					   }
					   break;
			case '<' : if( src[k+1] == '>' ) {
							if( dest[i-1] != ' ' ) {
								dest[i] = ' ';
								dest[++i] = '<';
								dest[++i] = '>';
								dest[++i] = ' ';
								i++;
							}
							else {
								dest[i] = '<';
								dest[++i] = '>';
								dest[++i] = ' ';
								i++;
							}
					   }
					   if( src[k+1] == '=' ) {
							if( dest[i-1] != ' ' ) {
								dest[i] = ' ';
								dest[++i] = '<';
								dest[++i] = '=';
								dest[++i] = ' ';
								i++;
							}
							else {
								dest[i] = '<';
								dest[++i] = '=';
								dest[++i] = ' ';
								i++;
							}
					   }
					   if( (src[k+1] != '>') && (src[k+1] != '=') ) {
							if( dest[i-1] != ' ' ) {
								dest[i] = ' ';
								dest[++i] = '<';
								dest[++i] = ' ';
								i++;
							}
							else {
								dest[i] = '<';
								dest[++i] = ' ';
								i++;
							}
					   }
					   break;
			case '>' : if( src[k+1] == '=' ) {
							if( dest[i-1] != ' ' ) {
								dest[i] = ' ';
								dest[++i] = '>';
								dest[++i] = '=';
								dest[++i] = ' ';
								i++;
							}
							else {
								dest[i] = '>';
								dest[++i] = '=';
								dest[++i] = ' ';
								i++;
							}
					   }
					   else {
							if( dest[i-1] != ' ' ) {
								dest[i] = ' ';
								dest[++i] = '>';
								dest[++i] = ' ';
								i++;
							}
							else {
								dest[i] = '>';
								dest[++i] = ' ';
								i++;
							}
					   }
					   break;
				default  : dest[i] = src[k];
					   i++;
					   break;
		}
	}
	dest[i] = ' ';
	dest[i+1] = '\0';
}

/*
 * Name    : express
 * Descrip : This is the math expression handler.  It takes a string and
 *           returns the numeric result, substituting variables as needed
 * Input   : form = pointer to a string to use as input
 * Output  : Returns the result on success, and calls err() on errors
 * Notes   : This is pretty simple, there is no operator precedence, and
 *           parenthesis are thrown away
 * Example : express( "3+3" ) - returns 6
 */

int express( char *form )
{
        int result = 0;             /* To hold the mathematical result */
	int i;
        char temp[MAXVARN];         /* To hold the parse()'d output ); */
	char buffer[MAXLINE];
        char op[2];                 /* Holds math operator (+,-,/,*) */

	explode( buffer, form );
	parse( buffer, 1, temp );
	result = getval( temp );
	if( numstr(buffer) > 1 ) {
		parse( buffer, 2, temp );
		strncpy( op, temp, 2 );
		for(i=3; i<=numstr(buffer); i+=2) {
			strset( temp, '\0' );
			parse( buffer, i, temp );
			switch( op[0] ) {
				case '+' : result += getval( temp );
						   break;
				case '-' : result -= getval( temp );
						   break;
				case '/' : result /= getval( temp );
						   break;
				case '*' : result *= getval( temp );
						   break;
				case '%' : result %= getval( temp );
						   break;
				case '&' : result &= getval( temp );
						   break;
				case '|' : result |= getval( temp );
						   break;
				case '^' : result ^= getval( temp );
						   break;
				default  : return( err( EXP_MATH, FATAL, op ) );
			}
			parse( buffer, i+1, op );
		}
	}
	return result;
}

/*
 * Name    : getval
 * Descrip : Gets the value of any numeric string, including variables
 * Input   : str = pointer to a string which either contains a variable name
 *           or the ASCII representation of a number
 * Output  : Returns the numeric version of the string
 * Notes   : Very simple.  Make sure variables start with a "#"
 * Example : getval( "#num" ) - returns value of "#num"
 *           getval( "998" )  - retruns 998 
 */

int getval( char *str )
{
	if( str[0] == '#' )
		return( getnvar(str) );
	return( atoi( str ) );
}

/*
 * Name    : test
 * Descrip : This is the conditional expression handler.  it checks all
 *           conditional expressions to see if they are true or not.
 * Input   : buffer = pointer to a string to use as input
 * Output  : Returns YES if the expression is true; NO if it isn't
 * Notes   : Handles all boolean expressions and numeric variables.
 * Example : test( "33 != 34" )     - returns YES
 *           test( "#score >= 10" ) - returns YES if "#score" is more than or
 *                                    equal to 10, otherwise NO
 */

int test( char *buffer )
{
	int i;
	int i2=0;
	int r1;
	int r2;
	char buf2[MAXLINE];
	char op[3];

	strset( buf2, '\0' );
	i = ws( buffer );
	if( toupper(buffer[i]) == 'Y' ) {
		if( getnvar("#MATCHED") == YES )
			return YES;
		else
			return NO;
	}
	if( toupper(buffer[i]) == 'N' ) {
		if( getnvar("#MATCHED") == NO )
			return YES;
		else
			return NO;
	}

	for(i=0; i<strlen(buffer); i++) {
		if( (buffer[i] == '=') || (buffer[i] == '>') || (buffer[i] == '<') ) {
			if( (buffer[i+1] == '=') || (buffer[i+1] == '>') ) {
				scopy( op, buffer, i, 2 );
				i2 = 1;
			}
			else {
				op[0] = buffer[i];
				op[1] = '\0';
			}
			break;
		}
	}
	if( i == strlen(buffer) )
		return( err( NO_RELAT, FATAL, "" ) );
	scopy( buf2, buffer, 0, i-1 );
	r1 = express( buf2 );
	if( i2 )
		scopy( buf2, buffer, i+2, strlen(buffer) - i+1 );
	else
		scopy( buf2, buffer, i+1, strlen(buffer) - i );
	r2 = express( buf2 );
	trim( op );

	if( strcmp("=", op) == 0 ) {
		if( r1 == r2 )
			return YES;
		else
			return NO;
	}
	if( strcmp(">", op) == 0 ) {
		if( r1 > r2 )
			return YES;
		else
			return NO;
	}
	if( strcmp("<", op) == 0 ) {
		if( r1 < r2 )
			return YES;
		else
			return NO;
	}
	if( strcmp("<>", op) == 0 ) {
		if( r1 != r2 )
			return YES;
		else
			return NO;
	}
	if( strcmp("<=", op) == 0 ) {
		if( r1 <= r2 )
			return YES;
		else
			return NO;
	}
	if( strcmp(">=", op) == 0 ) {
		if( r1 >= r2 )
			return YES;
		else
			return NO;
	}
}

/*
 * Name    : getstr
 * Descrip : Gets the value of the string variable "src"
 * Input   : dest = pointer to a string to store value of "src" in
 *           src = pointer to the string to use as the input
 * Output  : Puts the value of "src" into the string pointed to by dest
 * Notes   : This works just like getval(), but for strings
 * Example : 
 */

char *getstr( char *dest, char *src )
{
/*    int k;*/

	dest[0] = '\0';

	if( src[0] == '$' ) {
		getsvar( dest, src );
		return dest;
	}
    strcpy( dest, src );
/*
	for(k=0; k<strlen(src); k++) {
		if( src[k] == '\\' ) {
			switch( src[k+1] ) {
				case '\\' : strcat( dest, "\\" );
							break;
				case 'n'  : strcat( dest, "\n" );
							break;
				case '\"' : strcat( dest, "\"" );
							break;
				case 't'  : strcat( dest, "\t" );
							break;
			}
		}
	}*/
	return src;
}

/*
 * Name    : getlbl
 * Descrip : This gets the seek value of the label "name"
 * Input   : name = pointer to the name of a label
 * Output  : Returns the seek address of the label
 * Notes   : This is used with fseek() to implement jumps
 * Example : getlbl( "end" ) - returns offset of the "END" label
 */

long getlbl( char *name )
{
	struct lbl *prev;

	prev = lbl1;

	trim( name );
	strupr( name );
	while( prev != NULL ) {
		if( strcmp(prev->name, name) == 0 )
			return prev->line;
		prev = prev->next;
	}
}

