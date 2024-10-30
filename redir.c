//redir.c
//Jacob Gray
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <fcntl.h>

//Split the command and arguments
void split_command(char *cmd, char *argv[]) 
{
    char *token = strtok(cmd, " ");
    int i = 0;
    while (token != NULL) 
    {
        argv[i++] = token;
        token = strtok(NULL, " ");
    }
    argv[i] = NULL;
}

//Find Path of the executable
char *find_executable(char *cmd) 
{
    char *path = getenv("PATH");
    char *token = strtok(path, ":"); //Splits the PATH by ":"
    char full_path[1024]; 

    while (token != NULL) 
    {
        snprintf(full_path, sizeof(full_path), "%s/%s", token, cmd);
        if (access(full_path, X_OK) == 0) 
        {
            return strdup(full_path);
        }
        token = strtok(NULL, ":");
    }
    return NULL; //Command not found
}

int main(int argc, char *argv[]) 
{
    if (argc != 4) 
    {
        fprintf(stderr, "Usage: %s <inp> <cmd> <out>\n", argv[0]);
        exit(1);
    }

    char *input_file = argv[1];
    char *command = argv[2];
    char *output_file = argv[3];

    char *cmd_args[100];
    split_command(command, cmd_args); 

    //Command Path Check & Find
    if (cmd_args[0][0] != '/') 
    {
        char *full_path = find_executable(cmd_args[0]);
        if (full_path == NULL) 
        {
            fprintf(stderr, "Command not found: %s\n", cmd_args[0]);
            exit(1);
        }
        cmd_args[0] = full_path; //Command replaced with full path
    }

    pid_t pid = fork();
    if (pid < 0) 
    {
        perror("fork");
        exit(1);
    }

    //The Child Process
    if (pid == 0) 
    {
        //Input redirection
        if (strcmp(input_file, "-") != 0) 
        {
            int fd_in = open(input_file, O_RDONLY);
            if (fd_in < 0) 
            {
                perror("open input file");
                exit(1);
            }
            if (dup2(fd_in, STDIN_FILENO) < 0) 
            {
                perror("dup2 input");
                exit(1);
            }
            close(fd_in);
        }

        //Output redirection
        if (strcmp(output_file, "-") != 0) 
        {
            //Write only, create if not exist, truncate if exist (set to 0), Perms
            int fd_out = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0666);
            if (fd_out < 0) 
            {
                perror("open output file");
                exit(1);
            }
            if (dup2(fd_out, STDOUT_FILENO) < 0) 
            {
                perror("dup2 output");
                exit(1);
            }
            close(fd_out);
        }

        //Command Execution
        execv(cmd_args[0], cmd_args);
        perror("execv");
        exit(1);
    } 
    // Parent Process
    else 
    {
        int status;
        waitpid(pid, &status, 0);
        if (WIFEXITED(status)) 
        {
            return WEXITSTATUS(status); //Childs exit status
        }
    }

    return 0;
}
