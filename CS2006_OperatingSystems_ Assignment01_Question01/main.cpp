/*
    Muhammad Rehan
    22P-9106 | BSE-5A | CS2006 Operating Systems
    Assignment 01 | Question no 01

    So this question requires libreadline-dev,
    so dont forget to run
        -sudo apt-get install libreadline-dev

    How to Compile & Run:
        * gcc -o Question1 Question1.c -lreadline
        * ./Question1
*/

#include <stdio.h>
#include <stdlib.h>
// for string functions like strtok_r, strdup & strcmp
#include <string.h>
// for fork, chdir, execvp
#include <unistd.h>
// for waitpid()
#include <sys/types.h>
#include <sys/wait.h>
// for using errno.
#include <errno.h>
// for file operations like open, O_CREAT
#include <fcntl.h>
// for command line input and history reading
#include <readline/readline.h>
#include <readline/history.h>
// to check if the dir exits or not.
#include <sys/stat.h>

// defining no of arguments which could be passed to the commands.
#define MAX_ARG_SIZE 100

// Prototypes of the functions.
// Docs (Comments) of the functions are written below the main within them.
void execute_command(const char *input);

void change_directory(char *path);

void print_custom_error(const char *message);

void sigint_handler(int sig);

int handle_redirection(char *args[], int *redirect_type, char **redirect_file);

void tester();

int directory_exists(const char *path);

int main() {
    char *input;

    /*
     * Here we are installing a signal handler to
     * use Ctrl + C within our program, the program
     * will handle that signal without
     * closing suddenly.
     * */
    signal(SIGINT, sigint_handler);

    /*
     *  rather than running all the commands myself
     *  I am running this tester function to do this
     *  for me. consider this as a good practice of
     *  quality testing.
     * */
    tester();

    while (true) {
        /*
         * Here i am using readline()(its part of GNU Readline lib)
         * so when the user will press enter, it will  take the input,
         * but it will handle the command history and the line editing
         * itself.
         * */
        input = readline("rohtanza@debian> ");


        /*if the input is null, means the user has
         * pressed Ctrl + D in the empty line of the
         * terminal, in shell program this is used as
         * a signal to exit the shell, and loop will go berserk
         * */
        if (input == NULL)
            break;


        /*
         * if the user has press enter w/o
         * writing anything, so we are just
         * freeing the pointer and containing our
         * loop
         * */
        if (strlen(input) == 0) {
            free(input);
            continue;
        }

        /*
         * adding the current input to the shell history.
         * so it will help us negate through the commands.
         * using up and down arrow keys, making recall and
         * re-executing old commands.
         * */
        add_history(input);

        // if user enter exit, then just leave the program.
        if (strcmp(input, "exit") == 0) {
            printf("Exiting shell...\n");
            free(input);
            break;
        }

        // passing the command to our function to be executed.
        execute_command(input);

        // Freeing input memory
        free(input);
    }

    return 0;
}

/*
 *  Cant type each command myself, so
 *  making this fuction for the ease
 *  of the testing.
 * */
void tester() {
    printf("Running tester.....\n");

    // Testing mkdir, just checking if dir exists before creating
    printf("Test mkdir:\n");
    if (!directory_exists("test_dir")) {
        execute_command("mkdir test_dir");
    } else {
        printf("Directory 'test_dir' already exists, skipping mkdir.\n");
    }
    execute_command("ls");

    // Testing cd and pwd
    printf("\nTest cd and pwd:\n");
    execute_command("cd test_dir");
    execute_command("pwd");

    // Testing touch
    printf("\nTest touch:\n");
    execute_command("touch file1.txt");
    execute_command("ls");

    // Testing cp
    printf("\nTest cp:\n");
    execute_command("cp file1.txt file2.txt");
    execute_command("ls");

    // Testing mv
    printf("\nTest mv:\n");
    execute_command("mv file2.txt file_renamed.txt");
    execute_command("ls");

    // Testing rm
    printf("\nTest rm:\n");
    execute_command("rm file_renamed.txt");
    execute_command("ls");

    // Testing rmdir (after cleaning the dir)
    printf("\nTest rmdir (after cleaning directory):\n");

    // Removing the last file to clean the dir.
    execute_command("rm file1.txt");
    execute_command("cd ..");
    execute_command("rmdir test_dir");
    execute_command("ls");

    // Testing echo with redirection (and there are no quotes in echo)
    printf("\nTest echo with redirection:\n");
    execute_command("echo hello world > test_output.txt");

    // Displaying the file content to ensure "hello world" is in there
    execute_command("cat test_output.txt");

    // Testing grep
    printf("\nTest grep:\n");
    execute_command("grep hello test_output.txt");

    printf("\nTester completed.\n");
}

// Function to check if a directory exists
int directory_exists(const char *path) {
    struct stat st;
    if (stat(path, &st) == 0 && S_ISDIR(st.st_mode)) {
        return 1;
    }
    return 0;
}

/*
 * Function to execute commands,
 * its taking input as const, so
 * it could not be messed up within the
 * function.
 * */
void execute_command(const char *input) {
    /*
     * char *args array contains the
     * commands and its arguments,
     * and it can hold max of 100 arguments
     * */
    char *args[MAX_ARG_SIZE];
    // a ptr which will hold each token
    char *token;
    /*
     *  its is for strtok_r function,
     *  not using strtok() as that aint
     *  thread-safe as per google.
     *    - strtok_r keeps track of the
     *    - token b/w successive calls
     *    - to strtok_r
     * */
    char *saveptr;
    // argument count
    int arg_count = 0;

    /*
     *  this redirect_type will
     *  be used to keep track of
     *  the redirection like >, >> and
     *      * 1 for using > truncate mode
     *      * 2 for using >> append mode.
     *      * 0 for nothing.
     * */
    int redirect_type = 0;
    /*
     * if the input command will have any
     * redirection then the pointer below
     * will contain the name of the file
     * */
    char *redirect_file = NULL;

    // Tokenization process.
    /*
     * strdup(input) makes a duplicate of the input string
     * cause strtok_r modifies the string its working on
     * space " " is the delimiter on which we are tokenizing
     * the string, saveptr keeping track of the next position,
     * so it could keep tokenizing the string
     * */
    token = strtok_r(strdup(input), " ", &saveptr);

    // keeping the loop till we reached the end of the string or the max argu size.
    while (token != NULL && arg_count < MAX_ARG_SIZE - 1) {
        // adding current token to the array and +1ing the argument size
        args[arg_count++] = token;
        token = strtok_r(NULL, " ", &saveptr);
    }

    /* after the loop ends, we are
     * setting the las element of the
     * array to the null, as it is
     * required by many calls like execvp()
     * */
    args[arg_count] = NULL;

    // if Handle redirection function return means, no file
    if (handle_redirection(args, &redirect_type, &redirect_file) == -1) {
        // using fprintf has its looks cool then printf
        fprintf(stderr, "Error: Failed to handle redirection.\n");
        return;
    }

    // here I am handling the command to cd
    if (strcmp(args[0], "cd") == 0) {
        if (args[1] == NULL) // this case will handle if the user didn't mention the dir
            change_directory(NULL);
        else  // this case if he/she did mention a dir
            change_directory(args[1]);
        return;
    }

    /*
     *  This is a must to create a child
     *  process using a fork as shell doesn't
     *  execute the commands itself, So it
     *  needs a child process to run the command.
     * */
    pid_t pid = fork();
    if (pid == -1) {
        // means we couldn't create a child process.
        print_custom_error("The fork has failed");
        return;
    }

    if (pid == 0) {
        // if the redirection type is overwrite.
        if (redirect_type == 1) {
            /*Let's explain the code statement below:
             *      -open file with open system call:
             *      -redirect_file is the file name
             *      -O_WRONLY opens it in the write only mode
             *      -O_CREAT creates the file if it isn't there
             *      -O_TRUNC this overwrites the file
             *      -0644 open files with rw-r--r-- (read/write for owner)
             *          and read only for others)
             *       */
            int fd = open(redirect_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            // Couldn't open the file
            if (fd == -1) {
                // Printing our error msg
                fprintf(stderr, "Error: Failed to open file %s for writing: %s\n", redirect_file, strerror(errno));
                exit(EXIT_FAILURE);
            }
            /*
             *      WHY AM I USING DUP HERE?
             *          dup2 is a system call which duplicates
             *          the fd file descriptor, so it could replace
             *          the standard output (STDOUT_FILENO), which is
             *          fd 1., So I could redirect the standard outputs
             *          to the file, cause any output from command executed
             *          by the child process will go to the file rather
             *          than the terminal.
             * */
            dup2(fd, STDOUT_FILENO);
            // just closing the file descriptor.
            close(fd);
        } else if (redirect_type == 2) {
            // for the case of append redirection
            /*
             *  Same as above just the change is O_APPEND,
             *  which won't override the file, but will write
             *  starting from the end of the file.
             * */
            int fd = open(redirect_file, O_WRONLY | O_CREAT | O_APPEND, 0644);
            if (fd == -1) {
                fprintf(stderr, "Error: Failed to open file %s for appending: %s\n", redirect_file, strerror(errno));
                exit(EXIT_FAILURE);
            }
            // Same as above
            dup2(fd, STDOUT_FILENO);
            close(fd);
        }

        /*
         *      THIS IS THE MAIN PART OF THE CODE
         *
         *   Using execvp() here, system call which will
         *   replace the current process with a new one,
         *
         *      We are giving it the command name (args[0])
         *      and the args (array of the arguments), it will search
         *      the command in the PATH environment variable /bin or
         *      /usr/bin dirs. in fact the p in the execvp() stand for
         *      the path.
         *
         * */
        if (execvp(args[0], args) == -1) {
            // if it returns -1 means, couldn't run the command

            fprintf(stderr, "Error: Failed to execute the %s: %s\n", args[0], strerror(errno));
            // Exiting if our command execution fails
            exit(EXIT_FAILURE);
        }
    } else {
        /*
         *  Thi is the Parent process,
         *  waiting for the child to finish
         * */
        int status;
        /*
         *  this is the make the parent process wait
         *      -pid is the id of the child
         *      -&status is the ptr to the status var
         *      -0 means the parent will wait w/o any
         *          special conditions.
         * */
        if (waitpid(pid, &status, 0) == -1) {
            // if waitpid return -1, then this block of code will run
            if (errno == EINTR) {
                /*
                 *  EINTR, means the child process
                 *  is interrupted before by a signal, like
                 *  Ctrl + C,
                 * */
                fprintf(stderr, "Error: Command interrupted by signal.\n");
            } else {
                // so it fails for another reason
                print_custom_error("Waitpid failed");
            }
            // if waitpid() ran successfully
        } else if (WIFEXITED(status)) {
            // WIFEXITED(status) is a macro which gets exit status of child
            int exit_code = WEXITSTATUS(status);
            /*
             *  WIFEXITED return 0 if its successful, otherwise
             *  command was hit by an error or messed up.
             * */
            if (exit_code != 0) {
                fprintf(stderr, "Command exited with non-zero status: %d\n", exit_code);
            }
        }
    }
}

/*
 *  function is to detect the redirection
 *  in the command, it will identify the >
 *  or >> and the file it should be directed
 *  to
 * */
int handle_redirection(char *args[], int *redirect_type, char **redirect_file) {
    // this loop will be to find the redirect type and file name
    for (int i = 0; args[i] != NULL; i++) {

        // for the case of >
        if (strcmp(args[i], ">") == 0) {
            *redirect_type = 1;
            *redirect_file = args[i + 1];
            /*
             * Removing the ">" operator and file from args
             * to ensure the command pass to the execup(),
             * doesnt contain the >
             * */
            args[i] = NULL;
            break;
        } else if (strcmp(args[i], ">>") == 0) {

            // Same process as above
            *redirect_type = 2;
            *redirect_file = args[i + 1];
            args[i] = NULL;
            break;
        }
    }

    /*  ERROR HANDLING:
     *      - it is just to check if there was
     *      - a redirection type in the command
     *      - but the user forget to add the
     *      - file name.
     * */
    if (*redirect_type != 0 && *redirect_file == NULL) {
        // using fprintf has its looks cool then printf
        fprintf(stderr, "Error: There is no file provided for redirection.\n");
        return -1;
    }

    return 0;
}

// This function is to change the current working.
void change_directory(char *path) {
    if (path == NULL) {
        /*
         *  so if the path is null, means no argu for the dir,
         *  so i am using get environment variable function,
         *  so we will get the HOME evn var. which will have
         *  the path to the user current home dir.?
         *
         *      WHY I am not staying in the cur dir,
         *      cause of the shells, takes us back
         *      to the home dir with just using cd.
         * */
        path = getenv("HOME");
        if (path == NULL) {
            print_custom_error("cd: Unable to retrieve home directory.");
            return;
        }
    }
    /*
     *  Using the chdir system call to change
     *  the dir, by giving it the path, if it
     *  returns -1, means we messed up, and if
     *  the return value is zero, means we in the
     *  path dir.
     * */
    if (chdir(path) == -1) {

        /*
         *  Using the strerror with "errno",
         *  which is set by system call likes
         *  chdir when they fail, strerror will
         *  convert the error to a more readable
         *  string.
         * */
        // using fprintf has its looks cool then printf
        fprintf(stderr, "cd: %s: %s\n", path, strerror(errno));
    }
}

// the signal handler function for SIGINT (Ctrl+C)
void sigint_handler(int sig) {
    /*
     * this is to handle the signal,
     * and it helps us exit without
     * showing a msg
     * */
    printf("\nCaught signal %d (Ctrl+C). Type 'exit' to quit the shell.\n", sig);
    // Ensuring the output is displayed promptly
    fflush(stdout);
}

// Just a function to print custom error messages.
void print_custom_error(const char *message) {
    // using fprintf has its looks cool then printf
    fprintf(stderr, "Error: %s\n", message);
}
