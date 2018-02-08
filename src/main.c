#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include <limits.h>

#include <locale.h>

#define _GNU_SOURCE
#include <string.h>

#include <pthread.h>

#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <wait.h>
#include <time.h>

#include <unistd.h>
#include <getopt.h>

#include "rinp.h"

/**************************************************************/
/* three signal handler that they do nothing except returning */
/**************************************************************/

static void handle_SIGUSR1( int signal_number )
{
    // if you like to see when it is raised
    //write( STDOUT_FILENO, "SIGUSR1\n", 8 );
    return;
}

static void handle_SIGUSR2( int signal_number )
{
    // if you like to see when it is raised
    //write( STDOUT_FILENO, "SIGUSR2\n", 8 );
    return;
}

static void handle_SIGCHLD( int signal_number )
{
    // if you like to see when it is raised
    //write( STDOUT_FILENO, "SIGCHLD\n", 8 );
    return;
}

// this structure serves us iterating over a buffer easily.
// By joggling these three variables, we can parse any buffer
struct parseline
{
    // a pointer to a desired point, like --->'\n'
    char*   find_pos;

    // a pointer for updating the new position, like after '\n'<---
    char*   new_pos;

    // store difference between the 2 pointers value, like: how are you today?\n
    // from: -->h...?<-- or: find_pos - new_pos
    size_t  distance;
};

/*******************************/
/*     the main function       */
/*******************************/

int main( int argc, char** argv )
{
    if( argc == 1 )
    {
        help( *argv );
        exit( 0 );
    }

    // set standard output to un-buffered output
    setbuf( stdout, NULL );

    // for some uni-code characters
    setlocale( LC_ALL, "en_US.UTF-8" );

    /*******************************************/
    // registering SIGCHLD, SIGUSR1 and SIGUSR2 /
    /*******************************************/

    struct sigaction sigact;
    sigset_t new_mask, old_mask, zero_mask;

    sigemptyset( &sigact.sa_mask );
    //sigact.sa_flags   = SA_RESTART;
    sigact.sa_flags   = 0;
    sigact.sa_handler = handle_SIGUSR1;
    exitif( sigaction( SIGUSR1, &sigact, NULL ), "sigaction(2)", -1 );

    sigemptyset( &sigact.sa_mask );
    //sigact.sa_flags   = SA_RESTART;
    sigact.sa_flags   = 0;
    sigact.sa_handler = handle_SIGUSR2;
    exitif( sigaction( SIGUSR2, &sigact, NULL ), "sigaction(2)", -1 );

    sigemptyset( &sigact.sa_mask );
    //sigact.sa_flags   = SA_RESTART;
    sigact.sa_flags   = 0;
    sigact.sa_handler = handle_SIGCHLD;
    exitif( sigaction( SIGCHLD, &sigact, NULL ), "sigaction(2)", -1 );

    // block SIGUSR1 and SIGUSR2 for synchronization between master process
    // and new process(es) that (they) are going to be created.
    // laster on we will unblock these signals
    sigemptyset( &new_mask );
    sigaddset( &new_mask, SIGUSR1 );
    sigaddset( &new_mask, SIGUSR2 );
    sigaddset( &new_mask, SIGCHLD );
    exitif( sigprocmask( SIG_BLOCK, &new_mask, &old_mask ), "sigprocmask(2)", -1 );

    // creating two buffer
    // 1. line_buffer for storing all the main arguments that user passes
    // 2. main_buffer for storing all line of stdin (for example: < file) from user
    //    and combining/substituting the math-pattern {} with thus line(es)
    //
    // We first use line_buffer. After that with the help of this line_buffer
    // and reading -- a file or from PIPE by shell -- some lines and then
    // fill main_buffer and ending up it will contains all commands that should
    // be run by exec function.

    // buffer on the stack, for a single line (= all main args)
    char line_buffer[ RINP_LINE_MAX ];

    // buffer on the stack, for all commands/lines that ending up we will have
    char main_buffer[ RINP_LINE_MAX * RINP_LINE_MAX ];

    // zero out both buffers
    memset( line_buffer, '\0', RINP_LINE_MAX );
    memset( main_buffer, '\0', RINP_LINE_MAX * RINP_LINE_MAX );

    /************************************************************/
    /* Parsing main argument options with the help of getopt(3) */
    /************************************************************/

    // short option
    const char* short_option = "o:e:dlrvh";

    // long option
    const struct option long_option [] =
    {
        { "execute",    1, NULL, 'e' },
        { "output",     1, NULL, 'o' },

        { "draw-table", 0, NULL, 'd' },
        { "live",       0, NULL, 'l' },
        { "record",     0, NULL, 'r' },

        { "help",       0, NULL, 'h' },
        { "version",    0, NULL, 'v' },
        { NULL,         0, NULL,  0  }
    };

    int   result       = 0; // it gets the return value of getopt functin
    short finished     = 0; // it determines if we are finished or NOT
    short live_timer   = 0; // turn on or off the live-timer
    short record_timer = 0; // turn on or off the record-timer
    short draw_table   = 0; // turn on or off the draw-table

    // a fifo for output of new created process(es)
    const char* fifo_name = NULL;

    // a descriptor for this fifo
    int fifo_fd;

    // user input command, like, ffmpeg, cp, mv, sleep, etc
    char* lookfor = NULL;

    // what we should find in the PATH variable
    char* command = NULL;

    // clean any errors in getopt
    // opterr = 0;

    // parsing with long and short options
    while( ( result = getopt_long( argc, argv, short_option, long_option, NULL ) ) != -1 && !finished )
    {
        switch( result )
        {
        case 'e':
            // anything after -e flag is sotred in: line_buffer
            parse_main_arg( argv + ( optind - 1 ), line_buffer );

            // save the value of -e flag, this is the command we should check
            lookfor = optarg;

            // check for existence of a command with the help of command_exist()
            // if we can find it then proceeding the main function
            // otherwise exit with failure
            if( ( command = command_exist( lookfor ) ) == NULL )
            {
                // we could NOT find this command
                fprintf( stderr, "%s: pathname is empty or it does not exist\n", optarg );

                // exit with EXIT_FAILURE
                // its value is 1
                // also we can use 2 directly: exit( 2 ), and 2 means: No such file or directory
                // try: perror(1) 2
                // but this is the convention
                exit( EXIT_FAILURE );
            }

            // end of the while loop over main argument
            finished = 1;
            break;

        case 'o':
            // It may user have entered: -o -e
            if( *optarg == '-' )
            {
                fprintf( stderr, "invalid/empty name for your output\n" );
                exit( EXIT_FAILURE );
            }
            else
            {
                // store the name of FIFO
                fifo_name = optarg;

                struct stat fst;
                if( stat( fifo_name, &fst ) == -1 )
                {
                    pexit( "stat(2)", errno );
                }
                else
                {
                    // test it for its type.
                    // if it is not a FIFO, exit.
                    switch ( fst.st_mode & S_IFMT )
                    {
                        // if it is a FIFO then
                        case S_IFIFO:

                            // open(2) returns a new file discriptor or -1 for an error
                            if( ( fifo_fd = open( fifo_name, O_WRONLY  | O_NONBLOCK ) ) == -1 )
                            {
                                // in this case when we specifies O_NONBLOCK for open(2)
                                // if no process has opened this FIFO for reading then
                                // open(2) returns -1 and set error-number to ENXIO and that means
                                // no process has opened it for reading.
                                if( errno == ENXIO ) // may number 6
                                {
                                    perror( "open(2)" );
                                    printf( "\nYou did NOT open your FIFO for reading.\n"
                                            "Try: [cat %s](press Enter) on a separate terminal.\n", fifo_name );
                                    exit( errno  );
                                }
                                else
                                {
                                    pexit( "open(2)", errno );
                                }
                            }
                            break;

                        default:
                            fprintf( stderr, "Your (%s) is not is a FIFO.\n", fifo_name );
                            exit( EXIT_FAILURE );
                            break;
                    } // end of switch on fst.st_mode
                } // end else
            } // end of case 'o'
            break;

        case 'd':
            draw_table = 1;
            break;

        // live_timer case
        case 'l':
            live_timer = 1;
            record_timer = 0;
            break;

        case 'r':
            record_timer = 1;
            live_timer = 0;
            break;

        case 'h':
            help( *argv );
            break;

        case 'v':
            printf( "version: %s\n", RINP_VERSION );
            exit( EXIT_SUCCESS );
            break;

        case '?':
            fprintf( stderr, "error: -%c requires an argument\n", optopt );
            exit( EXIT_FAILURE );
            break;

        default:
            printf( "unknown error.\n" );
            exit( EXIT_FAILURE );
            break;
        } // end of switch
    } // end of while loop

    // line buffer should NOT be empty
    if( strlen( line_buffer ) == 0 )
    {
        fprintf( stderr, "nothing was provided to parse\n" );
        fprintf( stderr, "missing command-name\n" );
        exit( EXIT_FAILURE );
    }

    // Preparing main_buffer that is produced by line_buffer and stdin.
    // When a user provides the program an input from stdin liek < file
    //     then we already have line_buffer and now have stdin file that
    //     we should combine/substitute the match-pattern {} with each
    //     line in the file and eventually we have filled our main_buffer
    //     with several lines from a file/stdin and combining the match-pattern {}
    //     with that lines and our main_buffer is ready to be parsed and
    //     then run by exec* family.
    const size_t line_counter = parse_stdin( line_buffer, main_buffer );

    // Also it should NOT be grater than 1024
    if( line_counter > RINP_CHILD_MAX )
    {
        fprintf( stderr, "%d commands are not allowed\n", line_counter );
        fprintf( stderr, "It should be less or equal than %ld\n", RINP_CHILD_MAX );
        exit( EXIT_FAILURE );
    }

    // main buffer should NOT be empty
    if( strlen( main_buffer ) == 0 )
    {
        fprintf( stderr, "nothing was provided to parse\n" );
        fprintf( stderr, "standard input is empty\n" );
        exit( EXIT_FAILURE );
    }

    // This variable is used inside while(1) -- last one that tries waitpid(2)
    // We decrease the value of this variable after catching a SIGCHLD
    size_t number_of_process = line_counter;

    // just for readability to find out the maximum process(es) we have in while(1)
    // const
    const size_t limit       = line_counter;

    // Parse Main Buffer = pmb
    struct parseline pmb;
    pmb.find_pos = main_buffer;
    pmb.new_pos  = main_buffer;
    pmb.distance = 0;

    // set an array of pid_t
    // this array is equal or less than RINP_CHILD_MAX
    pid_t new_process[ number_of_process ];
    memset( new_process, 0, number_of_process );

    // A counter for fork(2)
    // It is controlled by master process (= parent)
    size_t pid_counter = 0;

    // We are going to parse main_buffer that contains '\n' newline.
    // Each line has a command in it, plus its argument that should be run
    // by exec* family. The first while() is for separating line(s).
    // The second while() is for separating each argument.
    //
    // Remember that at the beginning, we joined the main argument and
    // put to our: line_buffer and then create the main_buffer.
    // Now we parse it to extract each line and then extract each
    // argument for that command that should be run by exec* function
    while( ( pmb.find_pos = strchr( pmb.find_pos, '\n' ) ) != NULL )
    {
        // the distance from beginning up to '\n'
        pmb.distance = pmb.find_pos - pmb.new_pos;

        // By putting NUL character we treat this line a single string
        pmb.new_pos[ pmb.distance ] = '\0';

        // the two above expressions, could be:
        // pmb.new_pos[ pmb.find_pos - pmb.new_pos ] = '\0';
        // but it is not as readable as it should be

        // just for readability.
        // This variable is used for paring each line in the main_buffer
        char* line = pmb.new_pos;

        /********************************************/
        /*  preparing for spawning new process(es)  */
        /********************************************/

        // Array of pointer that we use for execvp(3)
        // The vector[ 0 ] will point to the command itself
        // and other parts are the arguments for the command
        // like: vector[ 0 ] ==> sleep
        //       vector[ 1 ] ==> 3
        //       vector[ 2 ] ==> NULL
        // the last index always will be NULL
        // We use this vector to avoid unnecessary copying from a buffer to another one.
        char*  vector[ RINP_ARG_MAX ];
        size_t vector_counter   = 0;

        // at this point we extracted a line from our main-bufer
        // now we parse this line
        // Parse Line: pl
        struct parseline pl;
        pl.find_pos = line; // or pmb.new_pos, line does nothing, it is only used for readability
        pl.new_pos  = line; // or pmb.new_pos, line does nothing, it is only used for readability
        pl.distance = 0;

        // line (= new_pos) has NUL-byte (= \0) at its end,
        // thus we can find its length
        const size_t line_length = strlen( line );

        // Start paring this line: (= pmb.new_pos)
        // The delimiter between the command and its argument is a single space: ' '.
        // We already made sure in our function trim_word() that we only have a single space and not more.
        while( ( pl.find_pos = memchr( line, ' ', line_length ) ) != NULL )
        {
            // the distance between each word.
            pl.distance = pl.find_pos - pl.new_pos;

            // like the above, we use this technique to avoid unnecessary copying
            pl.new_pos[ pl.distance ] = '\0';

            // the two above expressions, could be:
            // pmb.new_pos[ pmb.find_pos - pmb.new_pos ] = '\0';
            // but it is not as readable as it should be

            // okay, store a pointer to the current word that we just found.
            vector[ vector_counter ] = pl.new_pos;

            // skipping the space that we found
            pl.find_pos++;

            // update the new_pos and skip that single space
            pl.new_pos += pl.distance + 1;

            // the two above expressions, could be:
            // pl.new_pos += ( ++pl.find.pos );
            // but it is not as readable as it should be

            // increase our vector of pointers
            ++vector_counter;
        }
        // after this while loop, a word after last single space: ' '
        // is left and we should copy it as well.
        // at last one
        vector[ vector_counter ] = pl.new_pos;
        ++vector_counter;

        // We should make the last + 1 as NULL
        // because exec* function requires that its last argument should be NULL
        vector[ vector_counter ] = NULL;

        // Now at this point we have a vector of argument(s)
        // that we can pass to an exec function

        // for debugging purpose:
        // at this point we can print our vector contents

/*
       for( size_t index = 0; index < vector_counter; ++index )
       {
           printf( "(vector[%d]=%s) ", index, vector[ index ] );
       }

        putc( '\n', stdout );

        // plus we should skip newline
        ++pmb.find_pos;

        // and update new-position
        pmb.new_pos += ( pmb.distance + 1 );

        // at this point we go back to our while not to creating new process
        // by fork(2)


    } // end of while

} // end of main
*/

        // Here we have an array of pid_t and each fork(2) will fork for us.
        // Thus we will have an array for new created process(es).
        // The master process and new created process are synchronized
        // by SIGUSR1 and SIGUSR2 and -- these -- are sent back and forth between them.
        //
        // If we do not use this synchronization, still the program works,
        // because we are in while loop; and this while loop is controlled
        // by: strchr( pmb.find_pos, '\n' ) ) != NULL
        // but we may face some race condition between master process and
        // the new created process(es)

        // store the Process Identifier (= PID) or each newly created process in our array or pid_t
        switch( new_process[ pid_counter ] = fork() )
        {
            // handler error
            case -1:
            {
                pexit( "fork(2)", errno );
            }
            break;

            // handle new successful process(es) (= child)
            case 0:
            {
                // if we would like to see the synchronization between parent
                // and its child we can print:
                //printf( "I am the child and pid_counter %d\n", pid_counter );

                // Has the user entered -o option an a name?
                if( fifo_name != NULL )
                {
                    if( dup2( fifo_fd, STDOUT_FILENO ) == -1 )
                    {
                        perror( "dup2(2)" );
                        _exit( errno );
                    }
                    if( dup2( fifo_fd, STDERR_FILENO ) == -1 )
                    {
                        perror( "dup2(2)" );
                        _exit( errno );
                    }
                }
                // if not, then redirect the standard output, and error to /dev/null
                else
                {
                    if( ( fifo_fd = open( "/dev/null", O_WRONLY ) ) == -1 )
                    {
                        perror( "open(2)" );
                        _exit( errno );
                    }
                    dup2( fifo_fd, STDOUT_FILENO );
                    dup2( fifo_fd, STDERR_FILENO );
                }

                // process id of our parent
                const pid_t parent = getppid();

                // send signal to parent we are about to fire a command
                exitif( kill( parent, SIGUSR1 ), "kill(2)", -1 );

                // I only accept SIGUSR2
                sigemptyset( &zero_mask );
                sigfillset( &zero_mask );          // NOTE: it registers all signals
                sigdelset( &zero_mask, SIGUSR2 );  // then we remove SIGUSR2
                sigsuspend( &zero_mask );

                // run a command
                execvp( vector[ 0 ], vector );

                // should never occur
                // 1. because we already check for existence of the desired command
                // 2. if the argument of the command was not correct still we have run it.
                //
                // In fact this _exit happens only when the command can not be run
                // by exec* family, like: "sleep", "one", NULL.
                //
                // 'one' is not a valid argument for sleep, thus it can not be run
                // but we already created a new process
                // At this point two things happen:
                // 1. sleep produces its own error, but we redirected to /dev/null
                // 2. the exit status of this process should be 1 or failure
                //
                // If we did not specify our redirection to /dev/null then we would see
                // the output of any bad input for a valid command
                _exit( EXIT_FAILURE );
            }
            break;

            // handle master process (= parent)
            default:
            {
                // wait for a successful fork(2)
                // I only accept SIGUSR1
                sigemptyset( &zero_mask );
                sigfillset( &zero_mask );
                sigdelset( &zero_mask, SIGUSR1 );
                sigsuspend( &zero_mask );

                // okay, counts 1 newly created prcess
                ++pid_counter;

                // if we would like to see the synchronization between parent
                // and its child(ren) we can print:
                // printf( "I am the parent and pid_counter %d\n", pid_counter );
                // printf( "-----------------------------\n" );

                // tell the child we counted and it can go to run exec*
                exitif( kill( new_process[ pid_counter -1 ], SIGUSR2 ), "kill(2)", -1 );
            }
            break;
        }

        // skip the newline
        // this newline:
        // strchr( pmb.find_pos, '\n' ) ) != NULL
        ++pmb.find_pos;

        // update new-position
        pmb.new_pos += ( pmb.distance + 1 );

        // the two above expressions, could be:
        // pl.new_pos += ( ++pl.find.pos );
        // but it is not as readable as it should be

    } // end of while loop for parse main_buffer and spawn new process(es)

    /*********************************************************************/
    /* Print some details to stdout for the use, to see what is going on */
    /*********************************************************************/

    // For synchronization we blocked SIGUSR1 and SIGUSR2 and now we unblock
    // them. Set the signal mask to the old one
    exitif( sigprocmask( SIG_SETMASK, &old_mask, NULL ), "sigprocmask(2)", -1 );

    // Did user provide a FIFO or not?
    if( fifo_name == NULL )
    {
        // in this case user has not provided a FIFO thus the output is /dev/null
        printf( "output file      : \033[1;33m/dev/null\033[m\n" );
    }
    else
    {
        // we have a successfully opened FIFO
        printf( "your output file : \033[1;33m%s\033[m\n", fifo_name );
    }
    printf( "command to run   : \033[1;33m%s\033[m\n", command );
    printf( "master process ID: \033[1;33m%d\033[m\n", getpid() );
    printf( "sub-process(es)  : \033[1;33m%4d\033[m\n", number_of_process );
    printf( "\n" );

    // Do you remember that we find a desired command?
    // And here we used it, and no longer we will need it, thus is should be free(3)
    //
    // If the command has an absolute pathname && lookfor has an absolute path
    // then we should NOT use free(3).
    // because in the command_exist function we have not allocated from heap
    // In other words, user has entered a valid absolute past name like: /bin/sleep
    if( *command == '/' && *lookfor == '/' )
    {
        // nothing. user has entered an absolute path, like: /bin/sleep
        // and we have allocated nothing
    }
    // but if it is not an absolute pathname, we have allocated:
    // char* branch = calloc( sizeof( char ), RINP_LINE_MAX );
    // and we should release it
    else
    {
        free( command );
    }

    // draw a table if either live-timer or record-timer was enabled
    if( draw_table == 1 && ( live_timer || record_timer ) )
    {
        // title/header of the table
        const char* title = "elapsed time | process ID | return | status\n";
        const short title_length = strlen( title );

        const char* border = "-------------+------------+--------+-------\033[100D";
        const short border_length = strlen( border );

        write( STDOUT_FILENO, title, title_length );
        write( STDOUT_FILENO, border, border_length );

        // some symbols for the last column: status
        //                
        const char* table = "\033[1B             |            |        |    \033[100D";
        const short table_length = strlen( table );

        // draw the entire table once
        short row = limit;
        while( row-- )
        {
            write( STDOUT_FILENO, table, table_length );
        }

        // move the cursor the beginning of the table
        // but NOT when the input is only one line
        if( limit > 1 )
        {
            printf( "\033[%dA", limit - 1 );
        }
    }

    // fire a POSIX thread along with main thread, it prints a timer
    if( live_timer == 1  )
    {
        // thread ID
        pthread_t thid;

        // setting detached attribute to thid variable
        // We do not need that it be joinable
        pthread_attr_t attr;
        pthread_attr_init( &attr );

        // detached means: the life time of the thread is less or equal
        // then the main (= parent) thread and not more
        //
        // When the main thread exits, the helper (this one) will exit as well.
        // Thus we no need to join it later on.
        // ( we no need to call: pthread_join( thid, NULL );
        pthread_attr_setdetachstate( &attr, PTHREAD_CREATE_DETACHED );

        pthread_create( &thid, NULL, thread_timer, NULL );

        // We no need the attr anymore
        pthread_attr_destroy( &attr );
    }

    // clear zero_mask, for using it in sigsuspend(2)
    sigemptyset( &zero_mask );

    size_t millisecond = 0,
        second      = 0,
        minute      = 0,
        hour        = 0;

    // start time for -r record timer
    struct timeval start_time, end_time;
    gettimeofday( &start_time, NULL );


    // After the preceding while loop (= the above), we go to the while( 1 ) (= below)
    // and waiting for the signal SIGCHLD from the kernel, that notify us, that
    // a child has exited.
    // Then we get the terminated status of that process by waitpid(2) and print it.
    while( 1 )
    {
        // man 2 pause
        // pause() returns only when a signal was caught
        // and the signal-catching function returned.
        // In this case, pause() returns -1, and errno is set to EINTR.
        //
        // Thus we are going to sleep :) until a SIGCHLD is arrived
        // We do not care about 1 signal or any.
        //
        // The problem with signals is that they are asynchronous and
        // at any time may they arrive, If we had no simultaneously
        // terminated children, then we would have no problem and we can waitpid(2) them all.
        // But the reality is that may 10/100/1024 children have been terminated at the same time!
        //
        // If we want to catch them all with a single signal handler
        // of course may we lose/left some of them.
        //
        // And because of this, we should loop over the entire array that we
        // have (= array of pid_t). If 10 children exited at same time, the
        // kernel only delivers it once; the last one.
        //
        // When we receive that one, we iterate the entire of our array with: WNOWAIT flag
        // We could use pause(2), but I used sigsuspend(2)
        //
        // Suspend the master process at this point until a SIGCHLD is
        // sent by kernel, then sigsuspend(2) returns and we will have
        // a while() that loops over all indexes
        sigsuspend( &zero_mask );

        // we can use pause(2) as well
        //pause();

        size_t index = 0;
        while( index < limit ) // limit == number_of_process == line_counter
        {
            // We will assign any terminated process(es) to -2
            // in this way then we no need to wait for this one
            // of course I mean this index (that has -2 in it)
            if( new_process[ index ] == -2 )
            {
                ++index;
                continue;
            }

            // We are going to use waitpid(2)
            // and wait for any child; with no hang: WNOHANG flag
            //
            // man 2 waitpid
            // WNOHANG     return immediately if no child has exited.
            // note that the return value is: 0
            int wait_status;
            const pid_t return_pid =
                waitpid( new_process[ index ],  &wait_status, WNOHANG );

            // on error it return -1
            if( return_pid == -1 ) // AND TEST FOR NO CHILD
            {
                perror( "waitpid(2)" );
                break;

            }
            // when waitpid(2) -- first argument -- was specified by a
            // child-id and that child-id has not been terminated yet, in this
            // case the return value is 0
            // thus we ignore it by: if it is not equal (to) 0
            else
            if( return_pid != 0 )
            {
                if( WIFEXITED( wait_status ) )
                {
                    const short exit_status = WEXITSTATUS( wait_status );
                    if( live_timer == 1 )
                    {
                        if( exit_status == EXIT_FAILURE )
                        {
                            printf( "            " );
                            printf( "\033[3C%-10d\033[3C%-6d\033[5C\033[1;31m \033[m\n\033[100D", return_pid, exit_status );
                        }
                        else
                        {
                            printf( "\033[15C\033[1;33m%-10d\033[m\033[3C%-6d\033[5C\033[1;32m \033[m\n\033[100D", return_pid, exit_status );
                        }
                    }

                    if( record_timer == 1 )
                    {
                        gettimeofday( &end_time, NULL );

                        // get raw time in second
                        size_t raw_time = ( end_time.tv_sec - start_time.tv_sec );

                        // then update the millisecond
                        millisecond = ( ( end_time.tv_usec / 1000 ) - ( start_time.tv_usec / 1000 ) );

                        // sometimes it becomes negative and abs() is for fixing it
                        millisecond = abs( millisecond );

                        while( raw_time-- )
                        {
                            ++second;
                            if( second >= 60  )
                            {
                                ++minute;
                                second = 0;
                            }

                            if( minute >= 60 )
                            {
                                ++hour;
                                minute = 0;
                            }
                        }

                        if( exit_status == EXIT_FAILURE )
                        {
                            printf( "            " );
                            printf( "\033[3C%-10d\033[3C%-6d\033[5C\033[1;31m \033[m\n\033[100D", return_pid, exit_status );
                        }
                        else
                        {
                            printf( "%.2d:%.2d:%.2d:%.3d", hour, minute, second, millisecond );
                            printf( "\033[3C\033[1;33m%-10d\033[m\033[3C%-6d\033[5C\033[1;32m \033[m\n\033[100D", return_pid, exit_status );
                        }

                        // reset these variables for next process termination
                        second = 0;
                        minute = 0;
                        hour   = 0;
                    }
                }
                // abnormal exiting
                // this should NOT happen
                else
                {
                    printf( "abnormal exit!\n" );
                }

                // okay. This process has been terminated and we will set this
                // index to -2 as a flag that shows us later we
                // should skip this index from waiting for
                --number_of_process;
                new_process[ index ] = -2;
            }

            // the counter of: while( index < limit )
            ++index;

        } // end of: while( index < limit )

        // short and simple check for whether we should quit or wait
        if( number_of_process != 0 )
        {
            // go to sleep again
            continue;
        }
        else
        {
            // go out of this while(1)
            break;
        }

        // long and complex check for whether we should quit or wait
        /*
        // If all the indexes were -2, then we are done
        // and for check this, we loop over all the indexes
        // to determinate if all indexes are -2
        // otherwise we should go to sleep by pause() or sigsuspend(2) again
        size_t index2 = 0;
        while( index2 < limit )
        {
        // if we could find an index that its value is not -2, it means
        // we have an alive process-id and we should wait for it
        if( new_process[ index2 ] != -2 )
        {
        // okay. we found one index that is alive
        // thus we should go back and sleep until kernel notify us
        break;
        }
        // otherwise:
        ++index2;

        // check to see if we iterated over all indexes and all were -2 then
        // all children have been terminated and we no need to stay in
        // the while( 1 ) loop and we should finish the main function
        //
        // NOTE that we could ignore this while loop and would not use it.
        // We simply could use:
        // if( index1 == limit ) { exit; }
        // index1 is the counter for the first while loop: while( 1 )
        // Thus this extra while loop is just for making sure we have no un-terminated
        // process left.
        if( index2 == limit )
        {
        // for letting the thread_table print the last line and exit.
        if( record_timer == 1 )
        {
        putchar( '\n' );
        }

        printf( "finished\n" );
        exit( EXIT_SUCCESS );
        }

        } // end of: while( index2 < limit )
        */

    }// end of: while( 1 )


    printf( "\nfinished\n" );
    exit( EXIT_SUCCESS );

    // main return value
    return 0;
}

/************************/
/* end of main function */
/************************/
