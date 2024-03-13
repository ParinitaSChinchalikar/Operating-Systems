#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>

// limits
#define MAX_TOKENS 100
#define MAX_STRING_LEN 100
#define MAX_HISTORY_SIZE 100

// builtin commands
#define EXIT_STR "exit"
#define HISTORY "hist"
#define EXIT_CMD 0
#define UNKNOWN_CMD 99
#define TRUE 1
#define FALSE 0
#define WRITE_TO_FILE 1
#define READ_FROM_FILE -1
#define NO_REDIRECT 0

FILE *fp; // file struct for stdin
char **tokens;
char *line;

// Global Variables
int history_count = 0;
char *history[MAX_HISTORY_SIZE];
size_t MAX_LINE_LEN = 10000;
char *last_command = NULL;
int last_token_count = 0;

void initialize()
{

    // allocate space for the whole line
    assert( (line = malloc(sizeof(char) * MAX_STRING_LEN)) != NULL);

    // allocate space for individual tokens
    assert( (tokens = malloc(sizeof(char*)*MAX_TOKENS)) != NULL);

    // open stdin as a file pointer
    assert( (fp = fdopen(STDIN_FILENO, "r")) != NULL);

}

int tokenize (char * string)
{
    int token_count = 0;
    int size = MAX_TOKENS;
    char *this_token;

    while ( (this_token = strsep( &string, " \t\v\f\n\r")) != NULL)
    {

        if (*this_token == '\0')
            continue;

        tokens[token_count] = this_token;

        // printf("Token %d: %s\n", token_count, tokens[token_count]);

        token_count++;

        if(token_count >= size)
        {
            size*=2;
            // if there are more tokens than space ,reallocate more space
            assert ( (tokens = realloc(tokens, sizeof(char*) * size)) != NULL);
        }
    }
    return token_count;
}

int read_command()
{
    // getline will reallocate if input exceeds max length
    assert(getline(&line, &MAX_LINE_LEN, fp) > -1);

    // Adding the line of commands to an array
    if (history_count == MAX_HISTORY_SIZE)
    {
        for (int i = 1; i < history_count; i++)
        {
            history[i - 1] = history[i];
        }
        history_count--;
    }
    // Copy the command into history
    history[history_count] = strdup(line);
    history_count++;

    return tokenize(line);
}

int run_command(int token_count)
{
    if (strcmp(tokens[0], EXIT_STR) == 0)
        return EXIT_CMD;

    if (strcmp(tokens[0], HISTORY) == 0) 
    {
        // To print history of commands 
        for (int i = 0; i < history_count; i++)
        {
            printf("%d. %s", i + 1, history[i]);
        }
        return UNKNOWN_CMD;
    }

    if (strcmp(tokens[0], "!!") == 0)  // compare tokens with !! to execute last command
    {
        if (last_command == NULL)
        {
            printf("No previous command found!!!\n");
            return UNKNOWN_CMD;
        }
        token_count = last_token_count;
        strcpy(line, last_command);
        // printf("Last command is: %s\n", last_command);
    }
    else
    {
        if (last_command != NULL)
            free(last_command);

        last_command = strdup(line);
        last_token_count = token_count;
    }

    // To check if the command contains a pipe
    int pipe_index = -1;
    for (int i = 0; i < token_count; i++)
    {
        if (strcmp(tokens[i], "|") == 0)
        {
            pipe_index = i;
            break;
        }
    }

    if (pipe_index != -1)
    {
        char *first_command[MAX_TOKENS];  // command before pipe
        char *second_command[MAX_TOKENS]; // command after pipe 

        // Initializing the arrays
        for (size_t i = 0; i < MAX_TOKENS; i++) 
        {
            first_command[i] = NULL;
            second_command[i] = NULL;
        }

        // Copying the tokens before pipe to the first_command array
        for (int i = 0; i < pipe_index; i++)
        {
            first_command[i] = tokens[i];
        }

        // Copying the tokens after pipe to the second_command array
        int j = 0;
        for (int i = pipe_index + 1; i < token_count; i++)
        {
            second_command[j] = tokens[i];
            j++;
        }

        // Creating a pipe "|"
        int pipe_des[2];
        if (pipe(pipe_des) == -1)
        {
            printf("pipe error!!!");
            exit(EXIT_FAILURE);
        }

        pid_t pid1, pid2;
        pid1 = fork();

        if (pid1 < 0)
        {
            printf("Fork failed.\n");
            exit(EXIT_FAILURE);
        }
        else if (pid1 == 0)
        {
            // Child 1
            close(pipe_des[0]);  // Close the reading end of the pipe

            dup2(pipe_des[1], STDOUT_FILENO);  // Redirect the output to write end of the pipe

            execvp(first_command[0], first_command); 

            exit(EXIT_FAILURE);
        }
        else
        {
            // Parent process
            // Wait for child 1 to complete
            wait(NULL);

            pid2 = fork();

            if (pid2 < 0)
            {
                printf("Fork failed.\n");
                exit(EXIT_FAILURE);
            }
            else if (pid2 == 0)
            {
                // Child 2
                close(pipe_des[1]);   // Close the writing end of the pipe

                dup2(pipe_des[0], STDIN_FILENO);  // Redirect to take input from read end of the pipe

                execvp(second_command[0], second_command);

                exit(EXIT_FAILURE);
            }
            else
            {
                // Parent process
                // Close both ends of the pipe
                close(pipe_des[0]);
                close(pipe_des[1]);

                // Waiting for child 2 to complete
                wait(NULL);
            }
        }
    }
    else
    {
        int redirect_flag = NO_REDIRECT;
        char *filename = NULL;
        int redirect_type = NO_REDIRECT;

        // Check if the command contains ">" or "<"
        for (int i = 0; i < token_count; i++)
        {
            if (strcmp(tokens[i], ">") == 0)
            {
                redirect_flag = WRITE_TO_FILE;
                redirect_type = i;
                filename = tokens[i + 1];
                break;
            }
            else if (strcmp(tokens[i], "<") == 0)
            {
                redirect_flag = READ_FROM_FILE;
                redirect_type = i;
                filename = tokens[i + 1];
                break;
            }
        }

        if (redirect_flag == WRITE_TO_FILE)
        {
            // Split the command into two parts: before ">" and after ">"
            char *command[MAX_TOKENS];

            // Initialize the array
            for(size_t i = 0; i < MAX_TOKENS ; i++)
            {
                command[i] = NULL;
            }

            // Copy the tokens before ">" to the command array
            for(int i = 0; i < redirect_type; i++)
            {
                command[i] = tokens[i];
            }

            // Create a child process
            pid_t pid = fork();

            if (pid < 0)
            {
                printf("Fork failed.\n");
                exit(EXIT_FAILURE);
            }
            else if (pid == 0)
            {
                // Child process

                FILE *file = fopen(filename, "w");
                if (file == NULL)
                {
                    exit(EXIT_FAILURE);
                }
                dup2(fileno(file), STDOUT_FILENO); // Redirect the output to the file 

                execvp(command[0], command);  // Execute the command

                exit(EXIT_FAILURE);
            }
            else
            {
                // Parent process
                wait(NULL);
            }
        }
        else if (redirect_flag == READ_FROM_FILE)
        {
            // Split the command into two parts: before "<" and after "<"
            char *command[MAX_TOKENS];

            // Initialize the array
            for(size_t i = 0; i < MAX_TOKENS ; i++)
            {
                command[i] = NULL;
            }

            // Copy the tokens before "<" to the command array
            for (int i = 0; i < redirect_type; i++)
            {
                command[i] = tokens[i];
            }

            // Create a child process
            pid_t pid = fork();

            if (pid < 0)
            {
                printf("Fork failed.\n");
                exit(EXIT_FAILURE);
            }
            else if (pid == 0)
            {
                // Child process

                FILE *file = fopen(filename, "r");
                if (file == NULL)
                {
                    exit(EXIT_FAILURE);
                }
                dup2(fileno(file), STDIN_FILENO);  // Read the input from the opened file

                execvp(command[0], command);  // Execute the command

                exit(EXIT_FAILURE);
            }
            else
            {
                // Parent process
                wait(NULL);
            }
        }
        else
        {
            pid_t pid = fork(); // create a child process

            if (pid < 0)
            {
                printf("Fork failed.\n");
                exit(EXIT_FAILURE);
            }
            else if (pid == 0)
            {
                // Child process
                tokens[token_count] = NULL;    // Terminate the tokens array
                execvp(tokens[0], tokens);     // Execute the command
                exit(EXIT_FAILURE);
            }
            else
            {
                // Parent process
                wait(NULL);
            }
        }
    }

    return UNKNOWN_CMD;
}

int main()
{
    initialize();

    while (TRUE)
    {
        printf("sh550> ");

        int token_count = read_command();

        if (token_count == 0)
            continue;

        int result = run_command(token_count);

        if (result == EXIT_CMD)
            break;
    }

    free(line);
    free(tokens);

    return 0;
}