/*  Created by Mohsin Braer 
    May 9th, 2022
    Operating Systems Final Project */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <limits.h>

#define MAX_SUB_COMMANDS   5
#define MAX_ARGS           10

struct SubCommand {
    char *line;
    char *argv[MAX_ARGS];
};

struct Command {
        struct SubCommand sub_commands[MAX_SUB_COMMANDS];
        int num_sub_commands;

        //HW 4 fields
        char *stdin_redirect;
        char *stdout_redirect;
        int background;
    
};

void ResetValues(struct Command *command)
{
	command->stdin_redirect = "\0";
	command->stdout_redirect = "\0";
	command->background = 0;
    
    return; //add return to all functions to remove lag problem 

}

//HOMEWORK 2 MATERIAL

void ReadArgs(char *in, char **argv, int size) {
    char *token = strtok(in, " \n");
    int position = 0;
    char *sub_command;

    while(position < size-1 && token != NULL) {
        if (position == 10) {
            break;
        }
        sub_command = strdup(token);
        argv[position] = sub_command;
        token = strtok(NULL, " \n");
        position++;
    }
    argv[position] = NULL;

    return;
}

void PrintArgs(char **argv) {
    int i = 0;
    while(argv[i] != NULL) {
        printf("argv[%d] = '%s'\n", i, argv[i]);
        i++;
    }

    return;
}

int getSize(char **argv)
{
    int i = 0;
    while(argv[i] != NULL) {
        i++;
    }
    return i; 
}

//HOMEWORK 3 MATERIAL

void ReadCommand(char *line, struct Command *command) {
    char *token = strtok(line, "|");
    int position = 1;
    char *sub_command = strdup(token);
    command->num_sub_commands = 1;
    command->sub_commands[0].line = sub_command;


    while(position < MAX_SUB_COMMANDS) {
        token = strtok(NULL, "|");
        if(token == NULL){
            break;
        }
        sub_command = strdup(token);
        command->sub_commands[position].line = sub_command;
        position++;

        // if (*token == ' '){
        //     sub_command = strdup(token + 1);
        // } else {
        //     sub_command = strdup(token);
        // }
    }
    command->num_sub_commands = position; 

    int i;
    for(i = 0; i < command->num_sub_commands; i++) {
        ReadArgs(command->sub_commands[i].line, command->sub_commands[i].argv, MAX_ARGS);
    }

    return;
}



//Modified to meet HW4 requirments
void PrintCommand(struct Command *command) {
    int i = 0;
    for(i = 0; i < command->num_sub_commands; i++) {
        printf("Command %d: \n", i);
        PrintArgs(command->sub_commands[i].argv);
    }

    printf("\n");
    //Homework 4 printing material
    printf("Redirect stdin: %s\n", command->stdin_redirect);
    printf("Redirect stdout: %s\n", command->stdout_redirect);

    if(command->background == 1){ printf("Background: True\n");}
    else{printf("Background: False\n");}

    return;
}

//HOMEWORK 4 MATERIAL

void ReadRedirectsAndBackground(struct Command *command) //change so that to checks all subcommands 
{                                                       //REMOVE REDIRECT & BACKGROUND SYMBOLS AFTER USE
    ResetValues(command); //testing to make sure values are set to \0 and 0 

    char **lastCommand; 
    //lastCommand = command->sub_commands[command->num_sub_commands - 1].argv;
    //int sizeOfLastCommand = getSize(lastCommand)
    // if(strcmp(lastCommand[sizeOfLastCommand - 1], "&") == 0) 
    // {command->background = 1;}

    int i;  //MAX_ARGS - 1
    
    int k = 0;
    for(i = command->num_sub_commands - 1; i >= 0; i--){

       //char **currentCommand = command->sub_commands[i].argv;

        while(command->sub_commands[i].argv[k] != NULL)
        {
            if(strcmp(command->sub_commands[i].argv[k], ">") == 0) //output redirect
            {
                command->stdout_redirect = command->sub_commands[i].argv[k+1];
                command->sub_commands[i].argv[k] = NULL; //REMOVE
                k++; 
            } 
            else if(strcmp(command->sub_commands[i].argv[k], "<") == 0) //input redirect
            {
                command->stdin_redirect = command->sub_commands[i].argv[k+1];
                command->sub_commands[i].argv[k] = NULL; //REMOVE
                k++;
            }
            else if(strcmp(command->sub_commands[i].argv[k], "&") == 0)
            {
                command->background = 1;
                command->sub_commands[i].argv[k] = NULL; //REMOVE
                k++;
            }
            else
            { k++; }
        }
    }

    return;

}

//Fork one child process to exec the command given. Make sure to check if exec runs properly to check command validity
void CommandNoPipe(struct Command *command)
{
    pid_t ret;

    ret = fork();
    if (ret < 0)
    {
        perror("ERROR: fork failed");
    }
    else if(ret == 0) //child
    {
        if(command->stdin_redirect != "\0") //try using NULL instead
        {
            int fd = open(command->stdin_redirect, O_RDONLY, 0);
            if(fd < 0)
            {
                perror("ERROR: can not open file");
                exit(0);
             }


            dup2(fd, 0); // Make sure to use dup2 in order to pick fd 0
            close(fd);
        }
        if(command->stdout_redirect != "\0")
        {
            int fd1 = creat(command->stdout_redirect, 0644);
            if(fd1 < 0)
            {
                perror("ERROR: can not open file");
                exit(0);
            }

            dup2(fd1, 1);
            close(fd1);
        }

        int s = execvp(command->sub_commands[0].argv[0], command->sub_commands[0].argv);
        if(s < 0)
        {
           printf("%s: Command not found.\n", command->sub_commands[0].argv[0]); 
        }
    }
    else
    {
        if(command->background == 1) //only print process ID if background is set to true
        {
            printf("[%d]\n", ret);
            return;
        }
        else{
            wait(&ret);
            return;
        }
    }
}

//Make sure entire set of commands are redirected properly based on seperation with pipes
void CommandPipe(struct Command *command, int start) //See pipe HW
{
    int fds[2];

    //spawn children 
    pid_t child1, child2;

    int ret = pipe(fds);
    if(ret < 0)
    {perror("ERROR: pipe failed");}


    child1 = fork();
    if(child1 < 0)
    {perror("ERROR: child1 fork failed");}
    else if(child1 == 0)
    {
        close(fds[0]);
        close(1);
        dup(fds[1]);
        close(fds[1]);

        if(command->stdin_redirect != "\0") //Make sure to dup specific fd to a fd. USE dup2
		{
			int fd = open(command->stdin_redirect, O_RDONLY, 0);
			if(fd < 0)
			{perror("ERROR: can not open file"); exit(0);}
			dup2(fd, 0);
			close(fd);
		}
		if(command->stdout_redirect != "\0")
		{
			int fd1 = creat(command->stdout_redirect, 0644);
			if(fd1 < 0)
			{perror("ERROR: can not open file"); exit(0);}
			dup2(fd1, 1);
			close(fd1);
		}

        int s = execvp(command->sub_commands[start].argv[0], command->sub_commands[start].argv);
        if(s < 0)
		{
			printf("%s: Command not found\n", command->sub_commands[start].argv[0]);
		}

    }
    else{ // spawn child 2 fork in parent 

        child2 = fork();
        if(child2 < 0)
        {perror("ERROR: child 2 fork failed");}
        else if(child2 == 0) 
        {
            close(fds[1]); //opposite of child 1
            close(0);
            dup(fds[0]);
            close(fds[0]);

            if(command->stdin_redirect != "\0")
            {
                int fd2 = open(command->stdin_redirect, O_RDONLY, 0);
                if(fd2 < 0)
                {perror("ERROR: can not open file");
                exit(0);}
                dup2(fd2, 0);
                close(fd2);
            }
            if(command->stdout_redirect != "\0")
            {
                int fd3 = creat(command->stdout_redirect, 0644);
                if(fd3 < 0)
                {perror("ERROR: can not open file");
                exit(0);}
                dup2(fd3, 1);
                close(fd3);
            }

            int s = execvp(command->sub_commands[start + 1].argv[0], command->sub_commands[start + 1].argv);
            if(s < 0)
			{
				printf("%s: Command not found.\n", command->sub_commands[start + 1].argv[0]);
			}
        }
        else{
            
            close(fds[0]);
            close(fds[1]);
            if(command->background == 1)
            {
                printf("[%d]\n", child2);
                return;
            }
            else{
                int w1 = wait(NULL);
                int w2 = wait(NULL);
                if (w1 && w2)
                {return;}
            }

        }

    }
}

void CheckCommand(struct Command *command) // checks to see which method to use 
{
    if((command->num_sub_commands - 1) > 0)
    {
        int i;
        for(i=0; i < (command->num_sub_commands - 1); i++)
        {
            CommandPipe(command, i);
        }
    } else{
            CommandNoPipe(command);       
    }

    return;

}

int main() {
	char s[200];
    int status = 1;

	//printf("Enter a command: ");
    printf("\n\t\033[1m\033[36m...Welcome to the Shell...\033[0m\n");
    printf("\033[1m\033[33m   To exit the shell, please use CTRL C\033[0m\n");
    struct Command command;
    ResetValues(&command);

    do{
        sleep(1);
        char cwd[PATH_MAX];
        printf("\033[32m%s$\033[0m ", getcwd(cwd, sizeof(cwd)));
        fgets(s, sizeof s, stdin);
        //s[strlen(s) - 1] = '\0';

        ReadCommand(s, &command);
        ReadRedirectsAndBackground(&command);
        //Check Validity of Commands 
        CheckCommand(&command);
        //PrintCommand(&command);
        ResetValues(&command);


    }while(status);

    return 0;
}