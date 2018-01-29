#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include <limits.h>
#include <math.h>

#include <locale.h>

#define _GNU_SOURCE
#include <string.h>

#include <pthread.h>

#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>

#include <unistd.h>



// maximum argument for the main function
#define RINP_ARG_MAX   ( sysconf( _SC_ARG_MAX )   >= 100  ? 100 : sysconf( _SC_ARG_MAX ) )

// maximum number of running multi-commands simultaneously
#define RINP_CHILD_MAX ( sysconf( _SC_CHILD_MAX ) >= 1024 ? 1024 : sysconf( _SC_CHILD_MAX ) )

// maximum bytes of a line
#define RINP_LINE_MAX  ( sysconf( _SC_LINE_MAX )  >= 1024 ? 1024 : sysconf( _SC_LINE_MAX ) )

// maximum bytes of a PATH
#define RINP_PATH_MAX  ( sysconf( _PC_PATH_MAX )  >= 1024 ? 1024 : sysconf( _PC_PATH_MAX ) )


// this structure serves us iterating over a buffer easily.
// By joggling these three variables, we can parse any buffer
struct parseline
{
    // a pointer to a desired point, like --->'\n'
    char*   find_pos;

    // a pointer for updating the new position, like after '\n'<---
    char*   new_pos;

    // store difference between 2 the pointers value, like: how are you today?\n
    // from: -->h...?<-- or: find_pos - new_pos
    size_t  distance;
};

void pexit( const char* str, const int value )
{
    perror( str );
    exit( value );
}

void exitif( const int sys_ret, const char* name, const int user_want )
{
    if( sys_ret == user_want )
    {
        perror( name );
        if( errno == 0 )
        {
            // this case is for whne a function does not modify the errno but
            // we already know that its return value is an error. For example a
            // custom function that returns -1 but has not modified errno
            // In this case the errno == 0 but we have faced an error
            exit( EXIT_FAILURE );
        }
        else
        {
            exit( errno );
        }
    }
}

// for -l   --live live-timer
void* thread_timer( void* arg )
{
    struct timespec req;
    struct timespec rem;

    req.tv_sec  = 0;

    // 1e+9 = 1 second
    // 1e+6 = 1 millisecond
    // 1e+3 = 1 microsecond
    req.tv_nsec = 1e+6;

    rem.tv_sec  = 0;
    rem.tv_nsec = 0;

    struct timeval start_time, end_time;
    gettimeofday( &start_time, NULL );

    size_t millisecond = 0,
           second      = 0,
           minute      = 0,
           hour        = 0;

    while( 1 )
    {
        // sleep for 1 millisecond
        // if a signal interrupts nanoslee(2), the remaining time
        // is put to "&rem" and then we can use it to
        // complete the time for sleeping
        // you can see man 2 nanosleep
        if( nanosleep( &req, &rem ) == -1 )
        {
            pexit( "nanosleep(2)", errno );
        }

        // an alternate but not very accurate
        // usleep( 1000 );

        /// after one millisecond:
        // set end_time
        gettimeofday( &end_time, NULL );

        // update the second
        second      = ( end_time.tv_sec - start_time.tv_sec );

        // then update the millisecond
        millisecond = ( ( end_time.tv_usec / 1000 ) - ( start_time.tv_usec / 1000 ) );

        // sometimes it becomes negative and abs() is for fixing it
        millisecond = abs( millisecond );

        if( second == 60  )
        {
            ++minute;
            second = 0;

            // reset second elapsed time
            gettimeofday( &start_time, NULL );
        }
        if( minute == 60 )
        {
            ++hour;
            minute = 0;
        }

        printf( "%.2d:%.2d:%.2d:%.3d\r",
                hour, minute, second, millisecond );

    } // end of while( 1 )

    return NULL;
}

// a separate thread for draw a table, we can use this thread,
// but we also can use the main thread, in fact using main
// is simple and less complicated.
/*(
void* thread_table( void* arg )
{
    size_t millisecond = 0,
           second      = 0,
           minute      = 0,
           hour        = 0;

    int row = ( *( size_t* ) arg );
    int limit = row;

    //              
    const char* table = "\033[1B             |            |        |     \033[100D";
    const short table_length = strlen( table );

    while( row-- )
    {
        //sleep( 1 );
        write( STDOUT_FILENO, table, table_length );
    }

    // move the cursor the beginning of the table
    row = ( *( size_t* ) arg );
    printf( "\033[%dA", row );

    // record the start time
    struct timeval start_time, end_time;
    gettimeofday( &start_time, NULL );

    // go to sleep until the master thread (= main) notify you
    while( 1 )
    {
        // if we got a signal SIGUSER1
        if( pause() == -1 && errno == EINTR )
        {
            // record the end of time
            gettimeofday( &end_time, NULL );

            // update the second
            second      = ( end_time.tv_sec - start_time.tv_sec );

            // then update the millisecond
            millisecond = ( ( end_time.tv_usec / 1000 ) - ( start_time.tv_usec / 1000 ) );

            // sometimes it becomes negative and abs() is for fixing it
            millisecond = abs( millisecond );

            if( second == 60  )
            {
                ++minute;
                second = 0;

                // reset second elapsed time
                gettimeofday( &start_time, NULL );
            }
            if( minute == 60 )
            {
                ++hour;
                minute = 0;
            }

            // print this time to table
            printf( "\033[1B%.2d:%.2d:%.2d:%.3d\033[100D", hour, minute, second, millisecond );

            // one process was done
            --limit;
        }

        if( limit == 0 )
        {
            printf( "\n\nfinished\n" );
            exit( EXIT_SUCCESS );
        }

    } // end of while( 1 )

    return NULL;
}
*/

void parse_main_arg( char** argv, char* line_buffer )
{
    while( *argv != NULL ) // or while( *argv )
    {
        // If we want to check whether the line_bufer is empty or not
        // we should check it here, but there is no need to do it
        // because getopt-parse takes care of that for us and it does not
        // let user enter -e without its option that here it (=that option)
        // becomes our line_buffer
        size_t length = strlen( *argv );

        memcpy( line_buffer, *argv, length );
        line_buffer += length;

        // if we are about to end then we should add newline and NOT space
        if( *( argv + 1 ) == NULL )
        {
            memcpy( line_buffer, "\n", 1 );
            line_buffer++;
        }
        // otherwise a single space is required
        else
        {
            memcpy( line_buffer, " ", 1 );
            line_buffer++;
        }

        // forward the iterator
        ++argv;
    }
    // this function is void
}

void trim_word( char* buffer, const char* iterator )
{
    while( *iterator != '\0' ) // or just: while( *iterator )
    {
        // skip white-space (s)
        if( *iterator == ' ' )
        {
            ++iterator;
            continue;
        }
        else
        {
            // putc( *iterator, stdout );
            // copy from iterator to buffer and forward the buffer iterator
            *buffer = *iterator;
            ++buffer;

            // if we have white-space after and non-white-space(s) then we copy it once!
            // if the user enters extra white-spaces at the end like:
            // '            one              two                .          ' <--- here
            // then the last one is copied as well.
            // And we remove it after while loop
            if( *( iterator + 1 ) == ' ' )
            {
                // putc( ' ', stdout );
                // we need a single space between each character/word
                *buffer = ' ';
                ++buffer;
            }
        }

        ++iterator;
    }

    // at the end, if user has entered extra white-space(s) then we should trim them
    if( *( buffer - 2 ) == ' ' )
    {
        // make the extra white-space as newline
        *( buffer - 2 ) = '\n';

        // and NUL character and the end.
        *( buffer - 1 ) = '\0';
    }
}

size_t parse_stdin( char* line_buffer, char* main_buffer )
{
    // we are looking for this pattern in our line_buffer
    const char*  pattern = "{}";
    const size_t pattern_length = strlen( pattern );

    size_t newline_pos  = 0;
    size_t line_counter = 0; // how many line ended up we will have.

    if( strlen( line_buffer ) == 0 )
    {
        return line_counter; // or 0
    }
    else
    if( strstr( line_buffer, pattern ) == NULL )
    {
        // if we could not find the pattern {}
        // user wants to enter argument for the command with keyboard
        // not with pipe: | or not with bash < redirection from a file
        // Thus we find '\n' in our line_buffer that contains a valid command
        // like: -e 'sleep\n'
        // we convert \n to ' ' and copy 'sleep' (= the command) to lien_buffer
        newline_pos = strchr( line_buffer, '\n' ) - line_buffer;
        line_buffer[ newline_pos ] = ' ';
        // printf( "line_buffer: %s", line_buffer );
    }

    // parse line buffer = plb
    struct parseline plb;
    plb.find_pos = line_buffer; // pointer to beginning of the line_buffer
    plb.new_pos  = line_buffer; // pointer to beginning of the line_buffer
    plb.distance = 0 ;

    // variable for getline
    size_t  length = 0;
    char*   line   = NULL;  // should be free(3) at the end
    ssize_t read_bytes = 0; // retunred by getline(3)

    // loop over standard input
    while( ( read_bytes = getline( &line, &length, stdin ) ) != -1 )
    {

        // avoid empty line
        if( read_bytes == 1 )
        {
            printf( "empty line are not allowed\n" );
            break;
        }

        // if the newline_pos  was not zero, it means we have no pattern {} in our line_buffer
        // and that means user has not entred {}, thus we parse the line without a pattern
        // like: sleep < file.
        if( newline_pos != 0 )
        {
            // copy the line buffer to the main_buffer
            // + 1 is for extra single space ' ' that we added inplace of '\n'
            memcpy( main_buffer, line_buffer, newline_pos + 1 );
            main_buffer += newline_pos + 1;

            // temporary buffer
            char buffer[ RINP_LINE_MAX ];
            memset( buffer, 0, RINP_LINE_MAX );

            // remove any extra white-space
            trim_word( buffer, line );

            // then copy each line that user has entered
            const size_t buffer_length = strlen( buffer );
            memcpy( main_buffer, buffer, buffer_length );
            main_buffer += buffer_length;
        }
        else
        {
            // here we have pattern {} in line_buffer. like: sleep {} < file
            //
            // in this case because each [line] has newline we should not copy it
            // until all patterns {} were parsed for all line(s) that we have.
            if( line[ read_bytes - 1 ] == '\n' )
            // or
            // if( *( --line ) == '\n' )
            {
                // 1. eliminate that newline
                // 2. we should NOT copy it to our main_buffer because we want to create
                //    a string with a single NUl-character at the end; after parsing all patterns {}
                //    because if we have more than one patterns like: sleep {}.{} < file
                //    then we will have to replace these two patterns (= {}) with [line] input
                --read_bytes;
            }

            // loop over line_buffer to find pattern (= {}) and replace all of them with what
            // we have in [line] variable
            while( ( plb.find_pos = strstr( plb.find_pos, pattern ) ) != NULL )
            {
                // the distance between start and end of non-pattern characters
                plb.distance = plb.find_pos - plb.new_pos;

                // printf( "%.*s", plb.distance, plb.new_pos )
                // copy it to our main_buffer
                memcpy( main_buffer, plb.new_pos, plb.distance );
                main_buffer += plb.distance;

                // remove extra space around words
                char buffer[ RINP_LINE_MAX ];
                memset( buffer, 0, RINP_LINE_MAX );
                trim_word( buffer, line );

                // then copy each line that user has entered
                // BUT
                // we should NOT copy newline in this section because
                // if we have pattern {} then it has newline already
                // thus we ignore our newline in temporary buffer
                // -1 is for ignoring that \n
                const size_t buffer_length = strlen( buffer ) - 1;
                memcpy( main_buffer, buffer, buffer_length );
                main_buffer += buffer_length;

                /*
                memcpy( main_buffer, line, read_bytes );
                main_buffer += read_bytes;

                // If we have not used trim_word() function and just used a simple memcpy(3),
                // then if we had more than one white-space(s), then equal the size of those
                // white-space(s), args would produce for exec* function
                // like:
                // rinp -e sleep<-- Press Enter
                // 1           2 <- extra while-space
                // 3           4 <- extra white-space
                // ^D
                // exec would become:
                // exec( "sleep", "sleep", "1", "", "", "", "", "". "", "", "", "", "2" )
                //
                // xargs(1) does not handle this case.
                // You can try it by:
                // xargs -I xxx -P 0 sleep
                // 1                    3
                // sleep: missing operand
                // Try 'sleep --help' for more information.
                // 1                    2
                // sleep: missing operand
                // Try 'sleep --help' for more information.
                //
                // So, xargs(1) gives us error -- the output of sleep -- because it does not trim white-space
                // if it trimmed those extra white-spaces then sleep would run with:
                // sleep 1 3
                // sleep 1 2
                */

                // find_pos should skip the pattern_length which is 2 with {}
                plb.find_pos += pattern_length;

                // new_pos goes ahead to puts itself after a pattern
                // and ready for new parsing
                plb.new_pos  += plb.distance + pattern_length;

                // or just:
                // plb.new_pos += ( plb.find_pos += 2 )
                // but it is not as readable as it should be
            }

            // after the loop we may have some character after the last pattern
            // like: ffmpeg -i {} {}.mp3
            //
            // the above while loop only parses up to: ffmpeg -i {} {}
            // and then plb.find_pos becomes NULL and while loops ends.
            //
            // Then we will come out of the while() but the line_buffer still has: .mp3\n\0
            // Thus we should copy the reminder after that last pattern.
            // Since the plb.new_pos points just past the last pattern {}<---here
            // we will find its length and copy it to main_buffer to complete our iteration over a single line.
            //
            // strlen counts \n but does not count \0
            // and that is want we need
            // remember that we should add \n to the main_buffer and not \0 since we want to add
            // other lines (= commands) to this main_buffer. And main_buffer eventually is parsed
            // by these newline(s)
            size_t reminder = strlen( plb.new_pos ) ;
            memcpy( main_buffer, plb.new_pos, reminder ); // for example, copy: .mp3\n but NOT \0
            main_buffer += reminder;
            // printf( "%s and it is: %d", plb.new_pos, reminder );
        }

        // reset for the new iteration
        plb.find_pos = line_buffer;
        plb.new_pos  = line_buffer;
        plb.distance = 0;

        // count line
        ++line_counter;

    } // end of outer while loop

    // free line because it uses heap
    free( line );

    // return the number of line to the caller
    // so the caller know how many lines (= commads) should be run-in-parallel
    return line_counter;
}

char* command_exist( char* lookfor )
{
    // before doing anything, it may user passed a command with absolute path
    // like /bin/sleep
    if( *lookfor == '/' )
    {
        // okay, the command has absolute path and we should check whether
        // it exists or not
        struct stat fst;

        // get the statistic about the "lookfor", if it return -1, exit
        if( stat( lookfor, &fst ) == -1 )
        {
            // or: stat(2): No such file or directory
            return NULL;
        }
        else
        {
            // see man 2 stat
            switch (fst.st_mode & S_IFMT)
            {
                case S_IFBLK:   return NULL;
                case S_IFCHR:   return NULL;
                case S_IFDIR:   return NULL;
                case S_IFIFO:   return NULL;
                case S_IFLNK:   return NULL;
                case S_IFREG:   return lookfor; // okay it is a regular file
                case S_IFSOCK:  return NULL;
                default:        return NULL;
            }
        }
    }
    // here the fist character of lookfor did not have '/'.
    // in this case may user has entered something like: bin/sleep.
    // thus the lookfor should not cantaint any '/'
    else
    if( strchr( lookfor, '/' ) != NULL )
    {
        // the first character has not been '/' but we have it in middle
        // like: bin/sleep
        return NULL;
    }

    // At this point we have a single name like: sleep
    // First declare a pointer to this process PATH environment.
    // Then length of it because we have _PC_PATH_MAX and we should check for it
    char* const path            = getenv( "PATH" );
    const size_t path_length    = strlen( path );
    const size_t lookfor_length = strlen( lookfor );

    if( path_length > RINP_PATH_MAX )
    {
        fprintf( stderr, "You PATH length is too long: %d\n", path_length );
        exit( EXIT_FAILURE );
    }

    // for storing the absolute pathname
    char* branch = NULL;
    if( ( branch = calloc( sizeof( char ), path_length ) ) == NULL )
    {
        pexit( "calloc(3)", errno );
    }
    // if calloc was success then we proceed

    // an iterator for branch because we will use memcpy(3) and NOT strcpy and strcat
    char* branch_iterator  = NULL;
    /* const char* stat_error = NULL; */

    struct stat fst;

    // Parse Colon = pc
    struct parseline pc;
    pc.find_pos = path;
    pc.new_pos  = path;
    pc.distance = 0;

    // target is a single colon: ';'
    while( ( pc.find_pos = strchr( pc.find_pos, ':' ) ) != NULL )
    {
        // reset the iterator
        branch_iterator = branch;

        // measure the distance between the two pointer
        pc.distance = pc.find_pos - pc.new_pos;

        // we should NOT add NUL at to the PATH otherwise we have to duplicate it
        // because it is the only PATH variable of this process and we later on
        // use it and need it for exec call
        // pc.new_pos[ pc.distance ] = '\0';

        // zero out branch
        memset( branch_iterator, '\0', path_length );

        // copy a single path, like: /usr/bin to branch
        memcpy( branch_iterator, pc.new_pos, pc.distance );

        // increase the iterator with the length of distance
        branch_iterator += pc.distance;

        // add a single forward slash because we want to build an absolute path
        // like '/bin' + '/'
        memcpy( branch_iterator, "/", 1 );
        ++branch_iterator;

        // Then the lookfor variable, that eventually we have something like:
        // /bin/sleep
        memcpy( branch_iterator, lookfor, lookfor_length );

        // get statistic about the branch string, something like: /bin/sleep
        if( stat( branch, &fst ) == -1 )
        {
            // here an error occurred:
            switch( errno )
            {
            /* case EACCES: */
            /*     stat_error = "search premission is denided"; */
            /*     break; */

            /* case EBADF: */
            /*     stat_error = "bad file discriptor"; */
            /*     break; */

            /* case EFAULT: */
            /*     stat_error = "bad address"; */
            /*     break; */

            /* case ELOOP: */
            /*     stat_error = "too many symbolic links"; */
            /*     break; */

            /* case ENAMETOOLONG: */
            /*     stat_error = "pathname is too long"; */
            /*     break; */

            /* case ENOENT: */
            /*     stat_error = "pathname is empty or it does not exist"; */
            /*     break; */

            /* case ENOMEM: */
            /*     stat_error = "out of memory"; */
            /*     break; */

            /* case ENOTDIR: */
            /*     stat_error = "the path prefix of the pathname is not a directory"; */
            /*     break; */

            /* case EOVERFLOW: */
            /*     stat_error = "... number of blocks cannot be represneted"; */
            /*     break; */

            /* case EBADF: */
            /*     stat_error = "not a valid file-descriptor for a directory"; */
            /*     break; */

            /* case EINVAL: */
            /*     stat_error = "invalid falg"; */
            /*     break; */

            /* case ENOTDIR: */
            /*     stat_error = "path name is relative"; */

            } // end of switch( errno )

            // if you want to print the output:
            //printf( "stat_error: %s\n", stat_error );
        }
        else
        {
            // when we are here, it means stat(2) was success and
            // we should just determine the type of the input (= lookfor)
            // Is it a regular file?

            // if you want to print the output:
            // printf( "okay: %s\n", branch );

            // okay we found the pathname and it exists.
            // test it for its type:
            // for seeing what this switch does see: man 2 stat
            switch (fst.st_mode & S_IFMT) {
                case S_IFBLK:   free( branch ); return NULL; // block device
                case S_IFCHR:   free( branch ); return NULL; // character device
                case S_IFDIR:   free( branch ); return NULL; // directory
                case S_IFIFO:   free( branch ); return NULL; // FIFO/pipe
                case S_IFLNK:   free( branch ); return NULL; // symbolic link
                case S_IFREG:                 return branch; // regular file
                case S_IFSOCK:  free( branch ); return NULL; // socket
                default:        free( branch ); return NULL; // unknown
            }
        }

        // skip ':' that we found
        ++pc.find_pos;

        // update the new_pos, +1 is for colon: ':'
        pc.new_pos += pc.distance + 1 ;

        // or just:
        // pc.new_pos += ( ++pc.find_pos );
        // but it is not as readable as it should be

    } // end of while loop and strchr for ':'

    // when we are here it means command was not found
    free( branch );
    return NULL;
}

void help( const char* pn )
{
    printf(
        "%s  : Run in Paraleel.\n"
        "\n"
        "    -e    --execute    A valid command to run.\n"
        "    -o    --output     Redirect the output of your command to a FIFO.\n"
        "                       It should be opened for reading first.\n"
        "    -l    --live       show a live timer.\n"
        "    -r    --record     show a record timer.\n"
        "    -d    --draw-table Draw a table for a clean output.\n"
        "\n"
        "    -h    --help       (This) help.\n"
        "    -v    --version    Print version.\n"
        "\n"
        "NOTE.1: -e option should be applied at the end,\n"
        "         because the entire argument is parsed.\n"
        "    .2:  The only internal match-pattern is '{}'.\n"
        "    .3:  Both stdout and stderr of the command(s) are\n"
        "         redirected to /dev/null if you omitted the -o.\n"
        "    .4:  Internal limit to run command(s) is 1024.\n"
        "    .5:  Without a {}, each line is appended to the end.\n"
        "    .6:  Clear the screen before draw a table.\n"
        "\n"
        "quick examples\n"
        "      : echo -e '2\\n3\\n4' | rinp -e sleep\n"
        "      : echo -e '2\\n3\\n4' | rinp -re sleep\n"
        "      : echo -e '2\\n\\n4' | rinp -dre sleep\n"
        "      : echo -e '\\n3\\n' | rinp -dle sleep\n"
        "real examples\n"
        "      : ls *.mp4 | rinp -e ffmpeg -i {} {}.mp3\n"
        "      : cat list | rinp -e ffmpeg -i {} {}.mp3\n"
        "      : rinp -o your-fifo -e ffmpeg -i {} {}.mp3 < list\n"
        "\n"
        "copyright (C) 2017 Shakiba\n"
        "https://github.com/k-five/rinp\n"
        , pn
        );

    exit( EXIT_SUCCESS );
}
