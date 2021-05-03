// Your helper functions need to be here.
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include <unistd.h>
#include "icssh.h"
#include "helpers.h"
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>

int fd_In = 0;
int fd_Out = 1;
int fd_err = 2;
pid_t CHILD_ID = -1;
/******************************HELPER FUNCTION*****************************/
int countDirectories(char * path){ // return how many backslash exit in a path
	int count = 0;
	for(int i = 0;path[i]!='\0'; i++){
		if(path[i] =='/'){
			count++;
		}
	}
	return count;
}
char* copyStr(char* str){ //return copy of str 
	char *temp = (char*) malloc(500);
	for(int i = 0; str[i]!='\0'; i++){
		temp[i] = str[i];
	}
	temp[strlen(str)] = '\0'; 
	return temp;
}
int totalSpace(char* str){ // return how many spaces + 1 present in a strings
	int count = 0;
	for(int i = 0; str[i]!='\0'; i++){
		if(str[i] == ' '){
			count++;
		}
	}
	return count + 1;
}
char* concatinate(char* str1, char *str2){ // concatinate 2 strings and return result of both of them array
    int size1 = strlen(str1);
    int size2 = strlen(str2);
    char *temp = (char*)malloc(200*sizeof(char));
    
    int i = 0; // control temp index;
    for(; str1[i]!='\0'; i++){
        temp[i] = str1[i];
    }
    for(int j = 0; str2[j]; j++, i++){
        temp[i] = str2[j];
    }
    temp[i] = '\0';
    return temp;
}


char** parseCommand(char * userCommand){ 
	int space = totalSpace(userCommand);
	char **parse = (char**) malloc(100);
    int UCIndex = 0; //control userCommand array index
	int i = 0;
    // now parse argument if exist
    for(; i< space; i++){
        char * temp = (char*) malloc (100);
		int index = 0; //// control index of command_t->argv[i] array 
        while(userCommand[UCIndex] != ' ' && userCommand[UCIndex] !='\0'){
            temp[index] = userCommand[UCIndex];
            index++;
            UCIndex++;
        }
		temp[index] = '\0';
		parse[i] = temp;
        UCIndex++;
	}
	parse[i] = NULL;
    return parse;
}

char* removeCharacter(char* line , char c) { // remove character c from start or from end of the command
    char* end = NULL;
    char* start = line;

    while (*start == c)
        start++;

    for (end = line + strlen(line) - 1; end[0] == c ; end--) {
        end[0] = end[1];
    }
    end[1] = '\0';

    return start;
}

void removeFirstSpaces(char * str){ // it will remove spaces after pipe
	for(int i = 0; str[i] != '\0'; i++ ){
		if(str[i] == ' '){
			int size = strlen(str);
			for(int j = i; j< size; j++){
				str[j] = str[j+1];
				i--;
			}
		}
		else{
			return;
		}
	}

}

/**************************************************************************/
/*KIll all process*/
void allJobkill() {
    for(int i=0; i < back_g; i++){
        kill(jobs[i].PID, 9);
	}
}

void executeBgList(){
	for(int i = 0; i < back_g; i++){
		printf("\n[%d]  %d\t\t%s",i,jobs[i].PID,jobs[i].job_name);
	}
}



/*Execute CD command*/
int executeCd(char **argv) { //it will execute cd command
	if (strcmp(argv[0], "cd") == 0 ){ // if user enter just cd command 
		if(argv[1] == NULL){ // if user enter Just cd move at root
			char currentDirectory[500];
			getcwd(currentDirectory, 500); // get current directory
			for(int i = 0; i < countDirectories(currentDirectory);i++){
				chdir("..");               	
			}
			return 0;
		}
		else {// as cd take just 1 argument
			if(argv[3] == NULL){
				if (chdir(argv[1]) != 0 ) { //if chDir functionnot execute directly
					printf("\n%s",DIR_ERR);
				} 
			}
			return 0;
		}
	} 
	return 1;
}
//this function will return 0 for existance of file name
//retrun 1 for none existance of file name

void redirections(job_info* job){ //redirection >, <,
	fd_In = dup(STDIN_FILENO);
	fd_Out = dup(STDOUT_FILENO);
	fd_err = dup(STDERR_FILENO);
	
	if(job->procs->in_file!= NULL ){ // std in <
		int tempFd = open(job->procs->in_file, O_RDONLY); //open file in reading mode
		if(tempFd < 0){
			printf("%s",RD_ERR);
		}
		else {
			close(0);
			dup(tempFd); // swap tempfd with inputFd
		}
		job->procs->in_file = NULL;

	}
	else if(job->procs->out_file != NULL){ // std_out >  
		int tempFd = open(job->procs->out_file, O_CREAT|O_WRONLY, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH); //open file in writing mode
		if(tempFd < 0){
			printf("%s",RD_ERR);
		}
		else{
			close(1);
			dup(tempFd); // swap tempfd with outputFd
		}
		job->procs->out_file = NULL;
	}
	else if(job->procs->outerr_file != NULL ){ //error plus stdOut redirection &>
		int tempFd = open(job->procs->outerr_file, O_CREAT|O_WRONLY, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH); //open file in writing mode
		if(tempFd < 0){
			printf("%s",RD_ERR);
		}
		else{
			close(1); // close output FD
			dup(tempFd); // swap tempfd with outputFd
			close(2); // close error FD
			dup(STDOUT_FILENO);
		}
	}
	else if(job->procs->err_file != NULL){ //error redirection 2>  
		int tempFd = open(job->procs->err_file, O_CREAT|O_WRONLY, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH); //open file in writing mode
		if(tempFd < 0){
			printf("%s",RD_ERR);
		}
		else{
			close(2);
			dup(tempFd); // swap tempfd with outputFd
		}
	}
}

void resetRedirections(job_info* job){ // reset redirection <, >
	close(0);
	close(1);
	close(2);

	dup(fd_In);	
	dup(fd_Out);
	dup(fd_err);

	close(fd_In);
	close(fd_Out);
	close(fd_err);
}

char **pipe_elements(char *input) {
    char *p = strtok (input, "|");
    char **pipe_args = malloc(256 * sizeof(char *));

    int oof = 0;

    while (p != NULL) {
        pipe_args[oof++] = p;
        p = strtok (NULL, "|");
    }

    return pipe_args;
}

void piping(job_info *job, char *command) {
    char **pipe_args = pipe_elements(command);
    int pipes[2], fd = 0; pid_t proc;

    for(int j=0; pipe_args[j]!= NULL; j++) {
        pipe(pipes);
        proc = fork();
        
        if(proc < 0)
            perror("Nash");
        
        else if(proc > 0) {
            wait(NULL);
            close(pipes[1]);
            fd = pipes[0];
        }

        else if(proc == 0) {
            dup2(fd , 0);
            
            if(pipe_args[j+1]!=NULL) 
                dup2(pipes[1],1);
            
            close(pipes[0]);
			removeFirstSpaces(pipe_args[j]);
			recursiveExecute(job,pipe_args[j]);
            exit(2);
        }
    }
}

void run(job_info* job, char **args, int no_args, int bg) {
	int status;
	pid_t pid = fork(), wpid;
	args[no_args] = NULL;

	if (pid < 0) {    
		perror("Error");
		exit(EXIT_FAILURE);
	}

	else if (pid == 0)  {   
		setpgid(0, 0);
		if (execvp(args[0], args) < 0)  {     
			perror("Command not found");
			exit(EXIT_FAILURE);
		}
	}
	else {
		if(bg == 0) {
			CHILD_ID = pid;

			signal(SIGTTIN, SIG_IGN);
			signal(SIGTTOU, SIG_IGN);
			tcsetpgrp(STDIN_FILENO, pid);
			//signal(SIGTSTP, ctrl_z);
			wpid = waitpid(pid, &status, WUNTRACED);
			tcsetpgrp(STDIN_FILENO, getpgrp());
			signal(SIGTTIN, SIG_DFL);
			signal(SIGTTOU, SIG_DFL);

			if(WIFSTOPPED(status)) {
				printf("%s with PID %d suspended\n", job->line, pid);
				strcpy(jobs[back_g].job_name, job->line);
				jobs[back_g].PID = CHILD_ID;
				back_g++;
			}
		}

		else {
			strcpy(jobs[back_g].job_name, args[0]);

			for(int i = 1; i < no_args-1; i++) {
				strcat(jobs[back_g].job_name, " ");
				strcat(jobs[back_g].job_name, args[i]);
			}

			jobs[back_g].PID = pid;
			back_g++;
			printf("[%d] %d\n", back_g, pid);
        }
	}

	return;
}

int isPipingExist(char *str, char c){ // check is character c present in array or not
	for(int i = 0; str[i]!='\0'; i++){
		if(str[i] == c){
			return 1;
		}
	}
	return 0;
}

void recursiveExecute(job_info * job, char *command ){ //this function will execute cd, redirection operators, piping and background Jobs
	removeCharacter(command,'&');
	removeCharacter(command,' ');
	removeFirstSpaces(command);

	char *line = (char*) malloc(100);
	int i = 0;
	for(; command[i]!='\0'; i++){
			line[i] = command[i];
	}
	line[i] = ' '; i++;
	line[i] = '\0';

	char ** args = parseCommand(line);
    int no_args = totalSpace(line) - 1; // total number of arguments
    
    if(isPipingExist(line,'|')) { // check is pipe exist in str
        if( isPipingExist(line,'|') && job->nproc > 1 && job->nproc <= 3){ // if only 1 or 2 pipe exist in a command
			piping(job,line);
		}
		else{
			printf(PIPE_ERR);
		}
        return;
    }

    if(!strcmp(job->procs->argv[0], "cd")){
        executeCd(job->procs->argv);
	}
	else if(!strcmp(job->procs->argv[0], "bglist")){
		executeBgList();
		printf("\n");
	}
	else if(job->bg){
		run(job,args, no_args, 1);
	}
    else {
		run(job,args, no_args, 0);
	}
    for(int j=0; j < no_args != 0 && args[j] != NULL; j++) {
        free(args[j]);
	}
    free(args);  
}

int executeCommand(job_info * job){
	
	redirections(job);
	char * line = copyStr(job->line);
	for(int i = 0; line[i]!='\0'; i++ ){
		if(line[i] == '>' || line[i] == '<'){
			line[i] = '\0';
			break;
		}
	}
	recursiveExecute(job,line); //To solve piping problem
	return 0;
}
