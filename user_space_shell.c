// C Program to design a shell in Linux
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <errno.h>

#define MAXCOM 1000 // max number of letters to be supported
#define MAXLIST 100 // max number of commands to be supported

// Clearing the shell using escape sequences
#define clear() printf("\033[H\033[J")

//
/**
 * @brief Greeting shell during startup
 *
 */
void init_shell()
{
    clear();
    printf("\n\n\n\n******************"
           "************************");
    printf("\n\n\n\t****MY SHELL****");
    printf("\n\n\n\n*******************"
           "***********************");
    char *username = getenv("USER");
    printf("\n\n\nUSER is: @%s", username);
    printf("\n");
    sleep(1);
    clear();
}

//
/**
 * @brief Function to take input
 *
 * @param str - the command
 * @return int -success code
 */
int take_input(char *str)
{
    char *buf;

    buf = readline("\n>>> ");
    if (strlen(buf) != 0)
    {
        add_history(buf);
        strcpy(str, buf);
        return 0;
    }
    else
    {
        return 1;
    }
}

// Function to print Current Directory.
void print_dir()
{
    char cwd[1024];
    getcwd(cwd, sizeof(cwd));
    printf("\nDir: %s", cwd);
}

//
/**
 * @brief Function where the system command is executed
 *
 * @param parsed - the command
 */
void exec_args(char **parsed)
{
    // Forking a child
    pid_t pid = fork();

    if (pid == -1)
    {
        printf("\nFailed forking child..., error:%d", errno);
        return;
    }
    else if (pid == 0)
    {
        if (execvp(parsed[0], parsed) < 0)
        {
            printf("\nCould not execute command..., error:%d", errno);
        }
        exit(0);
    }
    else
    {
        // waiting for child to terminate
        wait(NULL);
        return;
    }
}

//
/**
 * @brief Function where the piped system commands is executed
 *
 * @param parsed - the parsed command before the pipe signal
 * @param parsedpipe - the parsed command afte the pipe symbol
 */
void exec_args_piped(char **parsed, char **parsedpipe)
{
    // 0 is read end, 1 is write end
    int pipefd[2];
    pid_t p1, p2;
    int status_code = 0;
    int wait_status;

    if (pipe(pipefd) < 0)
    {
        printf("\nPipe could not be initialized");
        return;
    }
    p1 = fork();
    if (p1 < 0)
    {
        printf("\nCould not fork");
        return;
    }

    if (p1 == 0)
    {
        // child process 1, only write

        close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[1]);
        if (execvp(parsed[0], parsed) < 0)
        {
            printf("\nCould not execute command 1..");
            exit(0);
        }
    }

    else
    {
        // Parent executing
        p2 = fork();

        if (p2 < 0)
        {
            printf("\nfork didn't not fork");
            return;
        }

        // child 2, only read
        if (p2 == 0)
        {
            close(pipefd[1]);
            dup2(pipefd[0], STDIN_FILENO);
            close(pipefd[0]);

            if (execvp(parsedpipe[0], parsedpipe) < 0)
            {
                printf("\nCould not execute command 2..");
                exit(0);
            }
        }
        else
        {
            close(pipefd[0]);
            close(pipefd[1]);
            // parent executing, waiting for two children
            wait_status = waitpid(p1, &status_code, 0);
            if (wait_status == -1 && errno != EINTR && errno != ECHILD)
            {
                fprintf(stderr, "wait failed in signal handler, error number %s\n", strerror(errno));
                exit(1);
            }
            wait_status = waitpid(p2, &status_code, 0);
            if (wait_status == -1 && errno != EINTR && errno != ECHILD)
            {
                fprintf(stderr, "wait failed in signal handler, error number %s\n", strerror(errno));
                exit(1);
            }
            // return;
        }
    }
}

//
/**
 * @brief Help command builtin
 *
 */
void open_help()
{
    puts("\n***WELCOME TO MY SHELL HELP***"
         "\nList of Commands supported:"
         "\n>cd"
         "\n>ls"
         "\n>exit"
         "\n>all other general commands available in UNIX shell"
         "\n>pipe handling"
         "\n>improper space handling");

    return;
}

//
/**
 * @brief Function to execute builtin commands
 *
 * @param parsed  - the parsed command
 * @return int - success code
 */
int command_handler(char **parsed)
{
    int i = 0, switchOwnArg = 0, found = 0;
    char *ListOfOwnCmds[] = {"exit", "cd", "help", "hello"};
    char *username;

    while (!found && i < sizeof(ListOfOwnCmds) / sizeof(char *))
    {
        if (strcmp(parsed[0], ListOfOwnCmds[i]) == 0)
        {
            switchOwnArg = i + 1;
            found = 1;
        }
        i++;
    }

    switch (switchOwnArg)
    {
    // case 1: exit
    case 1:
        printf("\nGoodbye\n");
        exit(0);
    // case 2: change directory
    case 2:
        chdir(parsed[1]);
        return 1;
    // case 3: open help
    case 3:
        open_help();
        return 1;
    // case 4: hello
    case 4:
        username = getenv("USER");
        printf("\nHello %s",
               username);
        return 1;
    default:
        break;
    }

    return 0;
}

//
/**
 * @brief  function for finding pipe
 *
 * @param str - the command
 * @param strpiped - the command pipe
 * @return int - success code
 */
int parse_pipe(char *str, char **strpiped)
{
    int i = 0;
    int found = 0;
    while (!found && i < 2)
    {
        strpiped[i] = strsep(&str, "|");
        if (strpiped[i] == NULL)
        {
            found = 1;
        }
        i++;
    }

    if (strpiped[1] == NULL)
        return 0; // returns zero if no pipe is found.
    else
    {
        return 1;
    }
}

/**
 * @brief function for parsing command words
 *
 * @param str - the command
 * @param parsed - the command post parsing
 */
void parse_space(char *str, char **parsed)
{
    int i = 0, found = 0;

    while (!found && i < MAXLIST)
    {
        parsed[i] = strsep(&str, " ");

        if (parsed[i] == NULL)
            break;
        if (strlen(parsed[i]) == 0)
            i--;
        i++;
    }
}
/**
 * @brief processes a command
 *
 * @param str - the command
 * @param parsed - parsed arguments
 * @param parsedpipe -parsed arguments for pipe
 * @return int - success code
 */
int process_string(char *str, char **parsed, char **parsedpipe)
{

    char *strpiped[2];
    int piped = 0;

    piped = parse_pipe(str, strpiped);

    if (piped)
    {
        parse_space(strpiped[0], parsed);
        parse_space(strpiped[1], parsedpipe);
    }
    else
    {

        parse_space(str, parsed);
    }

    if (command_handler(parsed))
        return 0;
    else
        return 1 + piped;
}
/**
 * @brief main function
 *
 * @return int - success code
 */
int main()
{
    char input_string[MAXCOM], *parsedArgs[MAXLIST];
    char *parsedArgsPiped[MAXLIST];
    int execFlag = 0;
    init_shell();

    while (1)
    {
        // print shell line
        print_dir();
        // take input
        if (take_input(input_string))
            continue;
        // process
        execFlag = process_string(input_string,
                                  parsedArgs, parsedArgsPiped);
        // 0 for no command, 1 for simple command, 2 for pipe

        // execute
        if (execFlag == 1)
            exec_args(parsedArgs);

        if (execFlag == 2)
            exec_args_piped(parsedArgs, parsedArgsPiped);
    }
    return 0;
}
