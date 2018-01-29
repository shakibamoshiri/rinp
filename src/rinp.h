#ifndef RINP_H
#define RINP_H

// maximum argument for the main function
#define RINP_ARG_MAX   ( sysconf( _SC_ARG_MAX )   >= 100  ? 100 : sysconf( _SC_ARG_MAX ) )

// maximum number of running multi-commands simultaneously
#define RINP_CHILD_MAX ( sysconf( _SC_CHILD_MAX ) >= 1024 ? 1024 : sysconf( _SC_CHILD_MAX ) )

// maximum bytes of a line
#define RINP_LINE_MAX  ( sysconf( _SC_LINE_MAX )  >= 1024 ? 1024 : sysconf( _SC_LINE_MAX ) )

// maximum bytes of a PATH
#define RINP_PATH_MAX  ( sysconf( _PC_PATH_MAX )  >= 1024 ? 1024 : sysconf( _PC_PATH_MAX ) )

#define RINP_VERSION "0.1.0"

// handle the live-timer
extern void*   thread_timer( void* arg );

// handle the statistic table
extern void*   thread_table( void* arg );

// parse the main function argument and put them in: line_buffer
extern void    parse_main_arg(  char** argv, char* line_buffer_iterator );

// time extra spaces in standard input of a file
extern void    trim_word( char* buffer, const char* iterator );

// parse standard input and put the result in main_buffer
extern size_t  parse_stdin( char* line_buffer, char* main_buffer );

// check for existence of a command
extern char*   command_exist( char* lookfor );

// perror(3) and exit together
extern void    pexit( const char* str, const int value );

// put the perror(3) and exit() and single line
extern void    exitif( const int sys_ret, const char* name, const int user_want );

// --help or -h
extern void    help( const char* pn );

#endif // RINP_H
