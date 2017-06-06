#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include<fcntl.h>
#include<errno.h>
#include<sys/types.h>
#include<sys/file.h>
#include<sys/wait.h>
#include<signal.h>
#include<pthread.h>

#include<ctype.h>
#include<time.h>

#define FILE_LENGHT 
#define CMD_ARGS_LENGHT 50
#define CMD_LENGHT 600


void getCommand(char* cmd);
void parseCmdString(char* cmd, char** Argv, char** Argv1, char* file, int* redirect, int* pipe);
int execCommand(char** Argv);
int execRedirect(char* file, char** Argv, int redirect);
int execPipe(char** Argv, char** Argv1);
int runPipe(int p[], char** Argv, char** Argv1);
void signalHandler();
void *thread_function(void *arg);


int main(){

    char* user = getenv("USER"); //login name
    char cmd[CMD_LENGHT+1]; //command given by user
    char* Argv[CMD_ARGS_LENGHT + 1]; // first command argument array
    char* Argv1[CMD_ARGS_LENGHT + 1]; // second command argument array
    char file[FILE_LENGHT + 1]; // stores a filename for redirects
    int redirect = -1; //flag for redirect to file
    int pipe = -1; //flag for pipe
    int score=0; //score of the game
    char input;
    int numberOfCommands;

    //loop until user types 'quit'
    do{
        printf("[%s]:-->", user);
   
        if(signal(SIGTERM, signalHandler)){ //ignore SIGTERM
            score=score-5;
            
        } 
        getCommand(cmd);//get the next command from user
        numberOfCommands=numberOfCommands+1; //count how many commands the user has given

        //when user types 'quit', ask him if he wants to play a game
        if(!strcmp(cmd,"quit")){
             do{
                //ask user if he wants to play a game and start threads if he answers YES
                puts("Do you want to play a slot machine game? Press Y if you do.\n");
                scanf(" %c", &input);
                
                    if(isalpha(input)==0){
                        printf("Please input something.\n");
                        continue;
                    }
                    if(input =='Y' || input =='y'){
                        int i,tmp;
                        int arg[numberOfCommands];
                        pthread_t thread[numberOfCommands];
                        pthread_attr_t attr;
                        //initialize and set thread attributes
                        pthread_attr_init(&attr);
                        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
                        //create threads
                        for(i=0;i<numberOfCommands;i++){
                            tmp = pthread_create(&thread[i], &attr, thread_function,(void *)&arg[i]);
                            if(tmp!=0){
                                fprintf(stderr, "Creating thread %d failed!\n",i);
                                return 1;
                            }
                            else{
                                char *array[3] = {"@", "#", "$"}; //array with the symbols for the slot machine
                                char *array2[3]; //second array where i save a random symbol from the first array
                                array2[0] = array[rand() % 3];
                                array2[1] = array[rand() % 3];
                                array2[2] = array[rand() % 3];
                                printf("%s%s%s\n", array2[0],array2[1],array2[2]);
                                //print a message if user won                            
                                if(array2[0]==array2[1] && array2[0]==array2[2] && array2[1]==array2[2]){
                                    printf("YOU WIN!!!!\n");
                                }
                                                
                            }

                        } 
                        printf("The final score is %d\n",score);
                        break;
                         


                            
                       
                    }
                    else{
                        printf("That's not Y\n");
                    }
                
            }
            while(1);
        }
        
        
        parseCmdString(cmd, Argv, Argv1, file, &redirect, &pipe); // parse input string in arguments

		// if there is a redirection
        if(redirect != -1){
            score=score+10;
            execRedirect(file, Argv, redirect);
            redirect = -1;
		// if there is a pipe
        }else if(pipe != -1){
            score=score+10;
            execPipe(Argv, Argv1);
            pipe = -1;
		// if it's just a simple command
        }else{
            score=score+10;
            execCommand(Argv);
        }
        
    }while(strcmp(cmd, "quit"));

    
   
        
   

}


void *thread_function(void *arg){

    int id,tmp;
    id = *((int *)arg);
        
    
    pthread_exit(NULL);
}

void signalHandler(){
    
    void(*oldHandler)();
    oldHandler = signal(SIGTERM, SIG_IGN); //ignore SIGTERM
    printf("\nSIGTERM ignored!\n");
    fflush(stdout);
}

void getCommand(char* cmd){

    fgets(cmd, CMD_LENGHT, stdin); // getting user's input

	//drop the newline that fgets reads
    if(cmd[strlen(cmd) - 1] == '\n'){
        cmd[strlen(cmd) - 1] = '\0';
    }
}


void parseCmdString(char* cmd, char** Argv, char** Argv1, char* file, int* redirect, int* pipe){
    int argc = 0; //array counter


    char* buffer = strtok(cmd, " "); //split the string into seperate strings

	
    while(buffer != NULL){
        Argv[argc] = buffer;//store argument 

		//if there is redirection
        if(!strcmp(Argv[argc], ">")){
            *redirect = 0; 
            Argv[argc] = NULL; //last argument as NULL (needed by execvp)
            buffer = strtok(NULL, " "); //take file name
            strcpy(file, buffer); //and store it
            break;
        }
		
		//if there is a pipe
        if(!strcmp(Argv[argc], "|")){
            (*pipe)++;
            Argv[argc] = NULL; //last argument again NULL
            parseCmdString(NULL, Argv1, NULL, NULL, NULL, NULL); //call the function to get the second argument
            break;
        }

        buffer = strtok(NULL, " "); // split again
        argc++;
    }


    Argv[argc] = NULL; //NULL at the last argument

}

int execCommand(char** Argv){

    pid_t pid = fork(); 

    
    if(pid == -1){
        char* error = strerror(errno);
        printf("fork: %s\n", error);
        return -1;
    }
    //Child process
    else if(pid == 0){
        execvp(Argv[0], Argv);//execute command

    }
    // Parent process
    else{
        
        //waiting child to finish and printing the command's exit code
        int status;
		if(waitpid(pid,&status,0) != -1 ){
			if(WIFEXITED(status)){
		
				int code = WEXITSTATUS(status);
				printf("The exit code is %d.\n",code);
			}
		}
    }


}

int execRedirect(char* file, char** Argv, int redirect){
    

    pid_t pid = fork();

    
    if(pid == -1){
        char* error = strerror(errno);
        printf("fork: %s\n", error);
        return -1;
    }
    //Child process
    else if(pid == 0){
		
		    
            
        if(redirect == 0){
            
            FILE *fd;
            fd=fopen(file,"w");
            dup2(fileno(fd), STDOUT_FILENO);
            fclose(fd);
            execvp(Argv[0], Argv);
        }

        
    }
    // Parent process
    else{
        //waiting child to finish and printing the command's exit code
        int status;
		if(waitpid(pid,&status,0) != -1 ){
			if(WIFEXITED(status)){
		
				int code = WEXITSTATUS(status);
				printf("The exit code is %d.\n",code);
			}
		}
    }
    
}
int execPipe(char** Argv, char** Argv1){
    int p[2];//READ&WRITE ends of pipe
    pid_t pid = fork();
    pipe(p);

    

    
    if(pid == -1){
        char* error = strerror(errno);
        printf("fork: %s\n", error);
        return -1;
    }
    //Child process
    else if(pid == 0){
		
        runPipe(p, Argv, Argv1);

    }
    // Parent process
    else{
        //waiting child to finish and printing the command's exit code
        int status;
		if(waitpid(pid,&status,0) != -1 ){
			if(WIFEXITED(status)){
		
				int code = WEXITSTATUS(status);
				printf("The exit code is %d.\n",code);
			}
		}
    }
}

int runPipe(int p[], char** Argv, char** Argv1){
    pid_t pid = fork();

    
    if(pid == -1){
        char* error = strerror(errno);
        printf("fork: %s\n", error);
        return -1;
    }
    //Child process
    else if(pid == 0){

        
        dup2(p[0], 0);
        close(p[1]);

        execvp(Argv1[0], Argv1);

        
    }
    // Parent process
    else{

        
        dup2(p[1], 1);
        close(p[0]);

        execvp(Argv[0], Argv);


    }
}

