/*
    Muhammad Rehan
    22P-9106 | BSE-5A | CS2006 Operating Systems
    Assignment 01 | Question no 02
*/

#include <stdio.h>
#include <stdlib.h>
// for fork, sleep and stuff
#include <unistd.h>
// for waitpid()
#include <sys/wait.h>
// for the time functions
#include <time.h>
// for the handling the variable arguments
#include <stdarg.h>

// The total number of the deployment stages
#define STAGES 4
// The maximum number of the retries for each stage
#define MAX_RETRIES 3

/*
 * Trying to make the log look cool, so
 * ANSI color codes for terminal output
 * while printing it
 * */
#define COLOR_GREEN "\033[32m"
#define COLOR_YELLOW "\033[33m"
#define COLOR_RED "\033[31m"
#define COLOR_RESET "\033[0m"

/*
 *  This is an array of the strings
 *  with names of our stages
 * */

const char *stages[STAGES] = {
        "System Update",
        "Software Installation",
        "Configuration Setup",
        "Security Checks"
};

/*
 * This function is storing the timestamp
 * in the buffer, size_t len is the size of
 * the buffer.
 */
void getTimestamp(char *buffer, size_t len) {
    // Getting the current time
    time_t now = time(NULL);
    /*
     * Here I am converting time to local time fashion
     * it is based on my machine timezone settings
     * */

    struct tm *t = localtime(&now);

    /*
     *  the strtime() just converts a struct of the
     *  time into a readable string and then storing it
     *  in the buffer, and I've also given it the format
     *  of the time.
     * */

    strftime(buffer, len, "%Y-%m-%d %H:%M:%S", t);
}

/*
 *
 *  This logMessage function is logging with a timestamp
 *  and color, (added both of these as they look cool)
 *  this function prints a message on the console and
 *  resets the color after it,
         * Parameters:
         *   - color: the asni color code
         *   - format: the message format
         *   - ...: just an additional parameter for formatted string
 */
void logMessage(const char *color, const char *format, ...) {
    /*
     *  va_list is a type provided by the
     *  C standard lib to carry a list of
     *  the arguments, when the number and
     *  the type is unknown, we will use it
     *  to process the extra arguments, when
     *  we will pass it to the log_message
     *  function
     * */
    va_list args;
    /*
     * va_start is the macro which will initializes
     * the va_list variable, to point to the first
     * optional argument in the function.
     * */
    va_start(args, format);

    // just making a array to store the timestamp
    char timestamp[20];
    // using getTimestamp() function to get the timestamp
    getTimestamp(timestamp, sizeof(timestamp));

    // printing the timestamp with the color
    printf("%s[%s] ", color, timestamp);

    /*
     * this vprintf() is a variadic version of the good
     * old printf(), it is used to print a formatted string
     * with the arguments are passed with the va_list.
     * */

    vprintf(format, args);
    // Resting color to default
    printf(COLOR_RESET);

    // va_end() is a macro which cleans the argument list (va_list)
    va_end(args);
}

/*
 *  The runStage function is to simulate a stage
 *  with a 15 parent change of failure, it uses
 *  sleep() for this purpose, and it randomly decides
 *  if your stage is failed

     *  Arguments:
     *   - stage: the index of the stage (0 to STAGES-1)
     * Returns:
     *   - 0 on the success, -1 on the failure

 */
int runStage(int stage) {
    // Logging the start of the stage
    logMessage(COLOR_GREEN, "Starting %s...\n", stages[stage]);

    // Simulating the work of each stage by sleeping for 1-3 seconds, we can increase it as much we want.
    sleep(rand() % 3 + 1);

    // Simulating a 15% chance of failure mentioned in the assignment instructions
    if (rand() % 100 < 15) {
        // Logging failure in the file
        logMessage(COLOR_RED, "%s failed.\n", stages[stage]);
        // if it fails then we are returning -1
        return -1;
    }
    // Logging success of the stage
    logMessage(COLOR_GREEN, "%s completed successfully.\n", stages[stage]);
    // if it works then returning the 0
    return 0;
}

/*
 * All of our control of the deployment is in the main function
 * here we are running the loop through each step, and retrying
 * all the max retires.
 */
int main() {
    // Initializing the random number generator on our curr time
    srand(time(NULL));
    int status;
    // An array to keep track of all the retries for each stage
    int retries[STAGES] = {0};


    // Here we are starting the log deployment
    logMessage(COLOR_GREEN, "Starting deployment process...\n");

    // here we are iterating through each of the stages
    for (int i = 0; i < STAGES; i++) {
        // A var to keep track if the stage succeeded or not.
        int success = 0;

        // Now lets try to run each stage, and we will retry up to MAX_RETRIES if the stage fail
        for (int attempt = 0; attempt <= MAX_RETRIES; attempt++) {
            // Forking a new child process for the stage
            pid_t pid = fork();

            if (pid < 0) {
                // here we are just checking if the fork failed
                perror("Fork failed");
                exit(EXIT_FAILURE);

            } else if (pid == 0) {
                // Now in the child process we will run the stage and will exit with its status.
                exit(runStage(i));
            } else {
                // Now in the parent process, we are waiting for the child to complete its process.
                waitpid(pid, &status, 0);

                // Checking if the child process is completed successfully or not
                if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
                    // it means stage was successful
                    success = 1;
                    // Moving to the next stage
                    break;
                } else {
                    // If the stage failed, we will just increment the retries and log a retry attempt in the log files
                    retries[i]++;
                    logMessage(COLOR_YELLOW, "Retrying again %s (%d/%d)...\n", stages[i], retries[i], MAX_RETRIES);
                }
            }
        }

        // And If in the last, the stage fails after the max retries, we will then abort the deployment
        if (!success) {
            logMessage(COLOR_RED, "Error: %s failed after %d attempts. Now terminating the deployment.\n", stages[i],
                       MAX_RETRIES);
            exit(EXIT_FAILURE);  // Terminate with failure status
        }
    }

    // If in the end, all our stages were completed successfully then we will log the success in the log file.
    logMessage(COLOR_GREEN, "\nDeployment has been completed successfully!\n");
    // printing retry stats for each stage
    for (int i = 0; i < STAGES; i++) {
        logMessage(COLOR_GREEN, "%s: %d retries\n", stages[i], retries[i]);
    }

    return EXIT_SUCCESS;
}
