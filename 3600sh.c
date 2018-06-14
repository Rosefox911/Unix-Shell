/**
 * CS3600, Spring 2013
 * Project 1 Starter Code
 * (c) 2013 Alan Mislove
 *
 * You should use this (very simple) starter code as a basis for
 * building your shell.  Please see the project handout for more
 * details.
 */

#include "3600sh.h"
#define USE(x) (x) = (x)
#define MAX_ARGS 100
#include <stdio.h>
#include <stdlib.h>

/**
This fucntion witll take a string (str), and return the next or first word that is contained in it.

The word is returned as a char *, and length is the length of the word.

Any whitespace in the beginning of str is ignored.

Special characters like "\t" and "\ " are supported, and are taken as part of the word.

arguments:

char *str             the string that is to be parsed into contituent words
int *ends             the point at which the new word ends; is useful for next invocation
char *word            the first word will be filled in this pre-allocated array.
int *length           this points to the length of the new word, and is set by this function. It must be allocated by the calling function.

return value:        1 if word is non-zero, 0 if there is no next word

*/

char ERR_UNREC_ESC[] = "Error: Unrecognized escape sequence.";
char ERR_AMPERSAND[] = "Error: Invalid syntax.";
char ERR_INOUTFILES[] = "Error: Invalid syntax.";
char ERR_UNABLETOOPENFILE[] = "Error: Unable to open redirection file.";

pid_t last_bkgd_process = 0;

int debug = 0;

int get_next_word(char *str, int *ends, char *word, int *length) {

    static int ampersand_detected = 0;
    if (str[0] == '\0' ) {
         ampersand_detected = 0;
         return 0;
    }

    /* first eat up all whitespace in the beginning of the str  */
    int i = 0;
    while( str[i] != '\0')  {
        if (str[i] == ' ' || str[i] == '\n'  || str[i] == '\t' ) {
           i++;
           continue;
        }
        else
            break;
    }
    /* in case the str only contained whitespace, we need to return  */
    if (str[i] == '\0' ) {
         ampersand_detected = 0;
         return 0;
    }

    int j = 0; // the destination counter
    /* now we are at the start of the next word */
    while( str[i]) {
        /* if the space is delimited by a backspace, consider it to be a part of the word */
        if (str[i] == '\\') {
             if(str[i+1] == ' ' || str[i+1] == 't' || str[i+1] ==  '&' ||  str[i+1] == '\\' ) {
                if (str[i+1] == 't')
                    word[j++]= '\t';
                else
                    word[j++]=str[i+1];
                i+=2;
                continue; /* carry on, cause this combination is part of the word, but do i+=2  */
             }
             else {
                printf("%s\n", ERR_UNREC_ESC);
                ampersand_detected = 0;
                return -1;
             }
        }

        if (str[i] == ' ' || str[i] == '\n'  || str[i] == '\t' )
            break;  /* we have reached the end of the current word  */
        else {
            // ampersand is a s pecial case
            if( str [i] == '&')
                 ampersand_detected++;
            word[j++] = str[i++];
        }
     }

    if(ampersand_detected)  {
        // ampersand should be the last character, otherwise its illegal commad
        if( word[j-1] != '&' || ampersand_detected > 1) {
            printf("%s\n", ERR_AMPERSAND);
            ampersand_detected  = 0;
            return -1;
        }
        if (j > 1) {         // if ampersand is part of a word, separate it out
            ampersand_detected = 0;
            j--; i--;
        }
    }
    word[j] = '\0';

    *length = j;
    if(*length == 0) {
        ampersand_detected = 0;
        return 0;
    }
    *ends = i;

    return 1;
}


/**

Parses argv so that its input, output and error files are initialized to supplied variables,
and so that argv is stripped of the input/output redirection commands from the argv list.

Before returning, it resets *argn to the new length. And populates argv with the remaining and true command line arguments.
Frees the memory for the in/out command strings.

infile, outfile and errfile should be allocated before they are passed to this function.

returns 0 on success, and non-zero in case there is some invalid format in the argv list
if it fails, it returns non-zero, but also free's up the strings pointed to by argv[]'s. but not argv itself.
*/
int process_inoutfiles(char **argv, int *argn, char *infile, char *outfile, char *errfile) {

    int i, err = 0, n = *argn;
    int redirection_found = 0; // keeps track if we have found a redirection yet
    int infound = 0, outfound = 0, errfound = 0;  // flags used internally to keep track of the in/out/err files in the command string

    // will be temporarily used to store the arguments except for the in/out redirection commands
	char **temp = (char **)malloc(sizeof(char **) * MAX_ARGS);
    int j = 0; // counter used for temp


    for( i = 0; i < n; i++) {
        if (!strcmp(argv[i], "<"))
        {
            if(infound || (i == (n -1))) { // proceed only if there is a single <, and it is not the last word
                err = 1;
                break;
            }
            else {
                if(!strcmp(argv[i+1], "<" ) || !strcmp(argv[i+1], ">" ) || !strcmp(argv[i+1], "2>" )) {
                    err = 1;
                    break;
                }

                strcpy( infile, argv[i + 1]);
                infound = 1;
                free(argv[i]);
                free(argv[i+1]);
                i++;    // jump one ahead, cause we have taken the next word as the input file
                redirection_found = 1;
                continue;
            }
        }
        if (!strcmp(argv[i], ">"))
        {
            if(outfound || (i == (n -1))) { // proceed only if there is a single >, and it is not the last word
                err = 1;
                break;
            }
            else {
                if(!strcmp(argv[i+1], "<" ) || !strcmp(argv[i+1], ">" ) || !strcmp(argv[i+1], "2>" )) {
                    err = 1;
                    break;
                }
                strcpy( outfile, argv[i + 1]);
                outfound = 1;
                free(argv[i]);
                free(argv[i+1]);
                i++;    // jump one ahead, cause we have taken the next word as the input file
                redirection_found = 1;
                continue;
            }
        }
        if (!strcmp(argv[i], "2>"))
        {
            if(errfound || (i == (n -1))) { // proceed only if there is a single >, and it is not the last word
                err = 1;
                break;
            }
            else {
                if(!strcmp(argv[i+1], "<" ) || !strcmp(argv[i+1], ">" ) || !strcmp(argv[i+1], "2>" )) {
                    err = 1;
                    break;
                }
                strcpy( errfile, argv[i + 1]);
                errfound = 1;
                free(argv[i]);
                free(argv[i+1]);
                i++;    // jump one ahead, cause we have taken the next word as the input file
                redirection_found = 1;
                continue;
            }
        }

        // according to requirements, there can be no arguments after redirection,
        // so if the redirection_found flad is set, then abort with "invalid syntax" error
        if(redirection_found) {
            err = 1;
            break;
        }

        temp[j++] = argv[i];
    }
    // now all the words without in/out redirection commands are contained in temp
    // now copy temp back to argv, so that the child process can be called with its various arguments

    if (j == 0) // j==0 is error, because it denotes nothing remains in the argument list
        return 1;

    if(err) {
        // we need to free the rest of the argv, cause we are aborting this commadn anyway
        for(;i<n;i++)
            free(argv[i]);
        return err;
    }

    *argn = j;      // this is the new length of original argv
    argv[j] = NULL;
    while(j--) {
        argv[j] = temp[j];
    }
    free(temp);
    return 0;
}

void freeargv(char **argv, int argn) {
    int k;
    for(k = 0; k < argn; k++)
        free( argv[k]);
    return;
}

int open_inouterrfiles(char *infile, char *outfile, char *errfile) {
    int fdin, fdout, fderr;
    if( strlen(infile)) {
        fdin = open(infile, O_CREAT | O_RDWR | O_TRUNC,  S_IRUSR | S_IWUSR);
        if(debug) printf("Opened input file %s\n", infile);
        if(fdin == 0) {
            printf("%s\n", ERR_UNABLETOOPENFILE);
            return -1;
        }
        int ret = dup2(fdin, 0);
        if (ret == -1){
            printf("%s\n", ERR_UNABLETOOPENFILE);
            return -1;
        }
    }

    if( strlen(outfile)) {
        fdout = open(outfile, O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
        if(debug) printf("Opened outpur file %s\n", outfile);
        if(fdout == 0) {
            printf("%s\n", ERR_UNABLETOOPENFILE);
            return -1;
        }
        int ret = dup2(fdout, 1);
        if (ret == -1){
            printf("%s\n", ERR_UNABLETOOPENFILE);
            return -1;
        }
    }

    if( strlen(errfile)) {
        fderr = open(errfile, O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
        if(debug) printf("Opened errput file %s\n", errfile);
        if(fderr == 0) {
            printf("%s\n", ERR_UNABLETOOPENFILE);
            return -1;
        }
        int ret = dup2(fderr, 2);
        if (ret == -1){
            printf("%s\n", ERR_UNABLETOOPENFILE);
            return -1;
        }
    }

    if(debug) printf("In open_inouterrfiles\n");
    return 0;
}

/**

Return  returns 0 on success in processing, non-zero on failure
**/
int process_line2(char *cmd)
{
    if(debug) printf("Processing line\n");

    char *arg = cmd;
    char word[MAXSTR];
    int ends = 0, length = 0;

	char **argv = (char **)malloc(sizeof(char **) * MAX_ARGS);
    int argn = 0;

    int ret;
    while ((ret = get_next_word(arg, &ends, word, &length))) {
		if (ret == -1) {
            freeargv(argv, argn);
            free(argv);
            return 0; // go to next command, this command is not valid
        }
		char *temp = (char *)malloc(sizeof(char)* MAXSTR);
        int i;
        for(i= 0; i < length; i++) {
           temp[i] = word[i];
        }
        temp[i] = '\0';

        arg = arg + ends; // for next invocation
        argv[argn++] = temp;
    }
    argv[argn] = NULL;

    if(argn == 0) {
        free(argv);
        return 0;
    }

    // if the user has typed "exit", then we need to exit the shell
    if( !strcmp(argv[0], "exit")) {
        freeargv(argv, argn);
        free(argv);
        return 1;  // non zero means exit
    }


    int k;
    if(debug) {
        for (k = 0; k < argn; k++)
            printf("%s,", argv[k]);
        printf("\n");
    }


    // if the first character, and the only character, of the last argument is ampersand, flag it
    int background = 0;
    if( argv[argn-1][0] == '&') {
        background = 1;
        // strip the ampersand from the arg list, cause it is not needed now, and helps in processing in/out files
        argn--;
        argv[argn] = NULL;
    }
    background = 2*background;

    char infile[MAXSTR] = "", outfile[MAXSTR] = "", errfile[MAXSTR] = "";
    // this function should change the argv, and populate in out and err files
    int inoutflag = process_inoutfiles(argv, &argn, infile, outfile, errfile);

    if(inoutflag)
    {
        free(argv);
        printf("%s\n", ERR_INOUTFILES);
        return 0;
    }

    if(debug) {
        printf("After calling process_inoutfiles\n");
        for (k = 0; k < argn; k++)
            printf("%s,", argv[k]);
        printf("\n");

        if(strlen(infile))  printf("infile = %s\n", infile);
        if(strlen(outfile))  printf("outfile = %s\n", outfile);
        if(strlen(errfile))  printf("errfile = %s\n", errfile);
        printf("\n");
     }

    // open_inouterrfiles(infile, outfile, errfile);


    if (debug) {
        for (k = 0; k < argn; k++)
            printf("%s,", argv[k]);
        printf("\n");
    }

    //int background = process_bkgd_ampersand(argv, argn]);

	pid_t child_pid;
	child_pid = fork();
	if (child_pid == 0) {
	    // If the process is to be a background process, put the new process in a
        // new process groupd. That way it wont use the terminal because each terminal
        // is associated with only one process group
        if (background)
            setpgid( 0,0);

        //int i = 0, k;
        //for (i = 0; i++; i< 10000*10000)   k = k*k;

        int ret = open_inouterrfiles(infile, outfile, errfile);
        if (ret) {
            exit(1);
        }

         // Req: In particular, if you are unable to find the requested exe-cutable, you should print
         // out Error: Command not found. Or, if the requested executable is not executable by
         //you due to a permissions error, you should print Error: Permission denied.

         // execv sets these flags
         // ENOENT The file filename or a script or ELF interpreter does not exist, or a shared library needed for file or interpreter cannot be found.
         // EACCES Execute permission is denied for the file or a script or ELF interpreter.
		if (execvp(argv[0], argv) == -1) {
            if (errno == ENOENT )
                printf("Error: Command not found.\n");
            else if (errno == EACCES)
                printf("Error: Permission denied.\n");
            else
                printf("Error: %s\n", strerror(errno));
		}
		exit(1);
		exit(1);
	} else {
	    // wait for the child if it is not a background process, otherwise continue
	    if (background) {
            waitpid(-1, NULL, WNOHANG);// WNOHANG means it returns immediately even if child is still running. -1 helps catch SIGCHLD for older background processes
            last_bkgd_process = child_pid;// later we are going to wait for this process to finish
	    }
		else
            waitpid(child_pid, NULL, 0); // wait for a normal child to exit (foreground chhild)
		}
    return 0;
}



int main(int argc, char *argv[]){

    // Code which sets stdout to be unbuffered
    // This is necessary for testing; do not change these lines
    USE(argc);
    USE(argv);
    setvbuf(stdout, NULL, _IONBF, 0);
    //Gets current username//
    char *username = getenv("USER");
    //Gets current dir//
    char dir[DIRMAX];
    getcwd(dir, DIRMAX);
    //Gets hostname//
    char hostname[DIRMAX] ="home";
    gethostname(hostname, DIRMAX);

    //Prints username, dir and host//

    //char * fixedword = (char *)malloc(sizeof(char)* DIRMAX);
    //char * end = (char *)malloc(sizeof(char)* DIRMAX);
    //int counter = 0;

 	char temparray[MAXSTR];
	const int tempsize = MAXSTR;



	while (1) {
		printf("%s@%s:%s> ", username ,hostname, dir);

        if( fgets( temparray, tempsize, stdin)) {
            if(process_line2(temparray))
                break;
        }
        else {
            //printf("didnt read anything\n");
            break;
        }

        if (feof(stdin))
            break;
	} //end while

    printf("So long and thanks for all the fish!\n");

    // wait for the last background process to finish before we return
    if(last_bkgd_process)
        waitpid(last_bkgd_process, NULL, 0); 

    return 0;
}

void do_exit() {
  printf("So long and thanks for all the fish!\n");
  exit(0);
}