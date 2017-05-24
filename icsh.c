/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/*
 * File:   syscall.c
 * Author: thienthaichotchai
 *
 * Created on May 13, 2017, 3:49 PM
 */

#define _POSIX_SOURCE
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdbool.h>
/*
 *
 */

void sigstop(int sig);

int FreeCommand = 0;
int LastestExit = 0;
int *stoparr[100];
int inputprocess = 0;

char** command_build(char* str){
    char **command;
    char delim[] = " ";
    char* token;
    int count = 0;
    int length = 0;

    for (token = strtok(str, delim); token; token = strtok(NULL, delim))
    {
        length += strlen(token);
        if(count == 0){
            command = malloc(sizeof(char*) * strlen(token)*100);
            command[count] = malloc(sizeof(char) * strlen(token));
            strcpy(command[count],token);
        }else{
            command[count] = malloc(sizeof(char) * strlen(token));
            strcpy(command[count],token);
        }
        count++;
    }
    strtok(command[count-1],"\n");
    FreeCommand = count;
    return command;
}

char** commandInput(){
    signal(SIGINT, SIG_IGN);
    signal(SIGTSTP, SIG_IGN);
    char *command[100];
    printf("icsh > ");
    fgets(command,100,stdin);
    inputprocess = 1;
    return command_build(command);
}

void sigstop(int sig){
    printf("Stop the process %d\n",getpid());
    kill(getpid(),SIGSTOP);
}

void newformat(){
    printf("icsh > ");
}

void kill_child(int sig){
    printf("\n");
    kill(2);
}

void free_command(char **cmd){
    for(int i = 0; i < FreeCommand;i++){
        free(cmd[i]);
    }
    free(cmd);
}

void redirect(char *source,char *des){
    int in;
    int out;
    size_t got;
    char buffer[1024];
    in = open (source, O_RDONLY);
    out = open (des, O_TRUNC | O_CREAT | O_WRONLY, 0666);

    if ((in <= 0) || (out <= 0))
    {
        fprintf (stderr, "Couldn't open a file\n");
        exit (errno);
    }

    dup2 (in, 0);
    dup2 (out, 1);
    close (in);
    close (out);

    while (1)
    {
        got = fread (buffer, 1, 1024, stdin);
        if (got <=0) break;
        fwrite (buffer, got, 1, stdout);
    }
        exit(0);
}

void command_list(char **prog_argv,int *process){
    signal(SIGTSTP,sigstop);
    if(strcmp(prog_argv[0],"ls") == 0){
        execvp("ls",prog_argv);
    }else if(strcmp(prog_argv[0],"count") == 0){
        int cnt = atoi(prog_argv[1]);
        for(int i = 1;i<=cnt;i++){
            sleep(1);
        }
        exit(0);
    }else if(strcmp(prog_argv[0],"echo") == 0){
        if(strcmp(prog_argv[1],"$?") == 0){
            printf("%d\n",LastestExit);
        }else{
            printf("%s\n",prog_argv[1]);
        }
        exit(3);
    }else if(strcmp(prog_argv[0],"sleep") == 0){
        int sec = atoi(prog_argv[1]);
        //for(int i = 0; i < 10; i++){
            //printf("count %d\n",i);
            sleep(sec);
        //}
        exit(0);
    }else if(strcmp(prog_argv[1],">") == 0){
        redirect(prog_argv[0],prog_argv[2]);
    }else if(strcmp(prog_argv[2],"<")){
        redirect(prog_argv[2],prog_argv[0]);
    }else if(strcmp(prog_argv[0],"exit") == 0){
                exit(2);
    }else if(strcmp(prog_argv[0],"jobs") == 0){
                exit(0);
    }else{
        // printf("no command found\n");
        // exit(0);
    }

}

void removeChar(char *str, char garbage) {

    char *src, *dst;
    for (src = dst = str; *src != '\0'; src++) {
        *dst = *src;
        if (*dst != garbage) dst++;
    }
    *dst = '\0';
}

void jobPrint(int *process,int *stateprocess){
    for(int i = 1;i<100;i++){
        if(stateprocess[i] == 1){
            printf("[%d] process %d is Stopped\n",i,process[i]);
        }else if(stateprocess[i] == 2){
            printf("[%d] process %d is Running\n",i,process[i]);
        }
    }
}

void fork_func(){
    char *command[100];
    char **prog_argv;
    int *process = calloc(100,sizeof(int));
    int *stateprocess = calloc(100,sizeof(int));
    int run = 1;
    int checkfin;
    int count = 1;
    int stopcount = 1;
    int jobcheck = 1;
    while(1){
        prog_argv = commandInput();
        int pid = fork();
        /*
         * Create a process space for the ls
         */
        if (pid < 0)
        {
          perror ("Fork failed");
          exit(errno);
        }
        else if (pid == 0)
        {
            signal(SIGINT,kill_child);
            if(strcmp(prog_argv[0],"ls") != 0 && strcmp(prog_argv[0],"jobs") != 0 && strcmp(prog_argv[0],"count") != 0
            && strcmp(prog_argv[0],"sleep") != 0 && strcmp(prog_argv[0],"echo") != 0 && strcmp(prog_argv[0],"fg") != 0
            && strcmp(prog_argv[0],"bg") != 0 ){

                if(prog_argv[1] != NULL){
                     if(strcmp(prog_argv[1],"<") == 0 || strcmp(prog_argv[1],">") == 0)
                            command_list(prog_argv,process);
                }
                  printf("-bash: %s: command not found\n",prog_argv[0]);
                  exit(0);
            }
            command_list(prog_argv,process);
        }
        else
        {
            int status;
            int i = 0;
            int background = 0;
            pid_t ret;
            while(prog_argv[i] != NULL){
                i++;
            }
            checkfin = waitpid(-1,&status,WNOHANG);
            while(checkfin != 0){
                for(int i = 0;i<100;i++){
                    if(process[i] == checkfin){
                        stateprocess[i] = 3;
                        printf("[%d] process %d is done\n",i,process[i]);
                    }
                }
                checkfin = waitpid(-1,&status,WNOHANG);
            }
            if(strchr(prog_argv[i-1],'&') != NULL){
                printf("[%d] %d\n",run,pid);
                stateprocess[run] = 2;
                process[run++] = pid;
                ret = waitpid(-1,&status,WNOHANG);
                //printf("return\n");
            }else{
                int stopid = pid;
                ret = waitpid(pid,&status,WUNTRACED);
                if(WEXITSTATUS(status) == 17){
                    process[run] = stopid;
                    stateprocess[run++] = 1;
                }
            }
            if(strcmp(prog_argv[0],"jobs") == 0){
                jobPrint(process,stateprocess);
            }

            if(strcmp(prog_argv[0],"exit") == 0){
                printf("\nBYE...\n");
                free(prog_argv);
                free(process);
                free(stateprocess);
                exit(2);
            }

            if(strcmp(prog_argv[0],"fg") == 0){
                int integer = atoi(strtok(prog_argv[1],"%"));
                if(kill(process[integer],SIGCONT) == 0){
                    waitpid(process[integer],NULL,0);
                    stateprocess[integer] = 0;
                }else{
                    printf("Error never stop this PID before...\n");
                }
            }

            if(strcmp(prog_argv[0],"bg") == 0){
                int integer = atoi(strtok(prog_argv[1],"%"));
                if(kill(process[integer],SIGCONT) == 0){
                    printf("[%d] %d\n",integer,process[integer]);
                    stateprocess[integer] = 2;
                    waitpid(process[integer],NULL,WNOHANG);
                }else{
                    printf("Error never stop this PID before...\n");
                }
            }

        }
        free_command(prog_argv);
        printf("\n");
  }
}

int main(int argc, char** argv) {

    printf("=== WELCOME TO MY SHELL ===\n\n");
    fork_func();
    return (EXIT_SUCCESS);
}
