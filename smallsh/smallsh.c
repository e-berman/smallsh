#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <error.h>
#include <err.h>
#include <signal.h>

char *
v_expansion(char *in, pid_t pid)
{

	char *pidstr;

	int n = snprintf(NULL, 0, "%jd", pid);
	pidstr = malloc((n + 1) * sizeof(*pidstr));
	sprintf(pidstr, "%jd", pid);

	return pidstr;

}

void exit_shell(int *pid_list)
{
	
	exit(0);	
	
}

void
status(int childStatus, int exit_status, int term_status)
{
	// if childStatus is run prior to non-built in exec, will return 0
	// otherwise returns the successful exit status, or aborted term signal code
	if (childStatus == -1) {
		printf("exit value 0\n");
	} else {
		if (WIFEXITED(childStatus)) {
			printf("exit value %d\n", WEXITSTATUS(childStatus));
		} else {
			printf("exit value %d\n", WTERMSIG(childStatus));
		}
	}
}

void
cd(char *arg1, char *arg2) 
{
	char *path;
	char *env;
		
	path = malloc(100 * sizeof(char));

	getcwd(path, 100);	

	// gets the cwd.
	// if no second arg present, will cd to home directory,
	// otherwise cd to path provided in arg2	
	if (arg2 != NULL) {
		chdir(arg2);
	} else {
		env = getenv("HOME");
		chdir(env);
	}
	
	free(path);	
	
	return;	
}

int
parse(char *in, char **args, int pid)
{

	int count = 0;
	int pid_length;
	int arg_count;
	char c;
	char next_c;
	char *pn;
	char tmp[2048];
	
	// initial command line output for shell loop						
	printf(": ");
	fflush(stdout);
	c = fgetc(stdin);
	count = 0;
	arg_count = 0;	

	// if comment line or new line entered, reprompts for next command.
	// Otherwise fills in buffer with loop that loops through each character of stdin.
	// I chose to use fgetc() as it seemed easier to parse when needing to store separate string arguments. 
	// Handles variable expansion if two $ are consecutive.
	// Will finally fill the args array when a space or EOF or \n is encountered.
	if (c == '#' || c == '\n')
	{ 
		if (c == '#') fgets(tmp, 2048, stdin);
	} else {
		while (c != EOF && c != '\n') {
			if (c == '$') {
				next_c = fgetc(stdin);
				if (next_c == '$') {
					pid_length = strlen(v_expansion(in,pid));
					strcat(in, v_expansion(in,pid));
					args[arg_count] = malloc((strlen(in)+1) * sizeof(char));	
					strcpy(args[arg_count], in);
					count += pid_length;
					free(v_expansion(in, pid));
					c = fgetc(stdin);
				} else if (next_c == EOF || next_c == '\n') {
					in[count] = c;
					args[arg_count] = malloc((strlen(in)+1) * sizeof(char));	
					strcpy(args[arg_count], in);
					args[arg_count+1] = (char*) NULL;
					c = next_c;
				} else {
					in[count] = c;
					in[count+1] = next_c;
					count += 2;
					c = next_c;
					c = fgetc(stdin);;
				}
			} else if (c != ' '){
				in[count] = c;
				count++;
				c = fgetc(stdin);
				if (c == EOF || c == '\n') {
					in[count] = '\0';
					args[arg_count] = malloc((strlen(in)+1) * sizeof(char));	
					strcpy(args[arg_count], in);	
					args[arg_count+1] = (char*) NULL;
				}
			} else {
				in[count] = '\0';
				args[arg_count] = malloc((strlen(in)+1) * sizeof(char));	
				strcpy(args[arg_count], in);
				arg_count++;
				count = 0;
				memset(in, 0, 2048);
				c = fgetc(stdin);
			}
		}	
	}
	
	arg_count++;			
	
	return arg_count;
}


int main(int argc, char *argv[])
{
	pid_t spawnpid = -5;
	pid_t pid = -1;
	int childStatus = -1;
	int childPid;
	int return_val;
	int arg_count;
	int exit_status;
	int term_status;
	int input_FD;
	int output_FD;
	int in_result;
	int out_result;
	int pid_count = 0;

	struct Command{
		char in[2048];
		char **arguments;
		int *pid_list;
	};
		
	struct Command user_input;
	
	// allocate memory for struct variables
	user_input.pid_list = malloc(512 * sizeof (char));
	user_input.arguments = malloc(512 * sizeof (char*));
	pid = getpid(); 	

	// continuous shell loop that parses user input, and then checks it
	// to see if any built in commands were entered (cd, exit, status).
	// If non-built in command entered, it will fork a new process to handle
	// the non-built in command.
	while(1)
	{

		arg_count = parse(user_input.in, user_input.arguments, pid);
		
		if (user_input.arguments[0] != NULL) {
		
			
			if (strncmp(user_input.in, "exit", 4) == 0) 
			{
				exit_shell(user_input.pid_list);
			}		 
			else if (strncmp(user_input.arguments[0], "cd", 2) == 0)
			{
				cd(user_input.arguments[0], user_input.arguments[1]);
			}		
			else if (strncmp(user_input.arguments[0], "status", 6) == 0) 
			{
				status(childStatus, exit_status, term_status);
			}	
			else
			{	
				// below code is partially modified from Canvas module: Executing a New Process
				spawnpid = fork();
				alarm(180);
				switch (spawnpid){
				case -1:
						perror("fork() failed!");
						exit(1);
						break;
				case 0:
						// checks for input/output redirection and & character for background processes
						// redirects if < or > are present, and if & is specified, dumps contents to /dev/null.
						if (arg_count > 1 && (strncmp(user_input.arguments[arg_count-1], "&", 1) == 0)) {
							printf("Child Process ID %d has begun", getpid());
							user_input.pid_list[pid_count] = childPid;
							pid_count++;

							output_FD = open("/dev/null", O_WRONLY);
							if (output_FD == -1) {
								perror("output to /dev/null failed");
								exit(1);
							}
							fcntl(output_FD, F_SETFD, FD_CLOEXEC);
							out_result = dup2(output_FD, 1);
							if (out_result == -1) {
								perror("output dup /dev/null failed");
								exit(2);
							}

							input_FD = open("/dev/null", O_RDONLY);
							if (input_FD == -1) {
								perror("input to /dev/null failed");
								exit(1);
							}
							fcntl(input_FD, F_SETFD, FD_CLOEXEC);
							in_result = dup2(input_FD, 1);
							if (in_result == -1) {
								perror("in dup /dev/null failed");
								exit(2);
							}
							user_input.arguments[arg_count-1] = (char*) NULL;
						}
						else if (arg_count > 2 && (strncmp(user_input.arguments[arg_count-2], ">", 1) == 0)) {
							
							output_FD = open(user_input.arguments[arg_count-1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
							if (output_FD == -1) { 
								perror("output file open()"); 
								exit(1); 
							}
							fcntl(output_FD, F_SETFD, FD_CLOEXEC);
							out_result = dup2(output_FD, 1);
							if (out_result == -1) { 
								perror("output dup2()"); 
								exit(2);
							}			
							user_input.arguments[arg_count-2] = (char*) NULL;
						}
						else if (arg_count > 2 && (strncmp(user_input.arguments[arg_count-2], "<", 1) == 0)) {
							input_FD = open(user_input.arguments[arg_count-1], O_RDONLY);
							if (input_FD == -1) { 
								perror("input file open()"); 
								exit(1); 
							}
							fcntl(output_FD, F_SETFD, FD_CLOEXEC);
							in_result = dup2(input_FD, 0);
							if (in_result == -1) { 
								perror("input dup2()"); 
								exit(2); 
							}
							user_input.arguments[arg_count-2] = (char*) NULL;
						}	
						
						// exec function is run based on adjusted parameters from redirection and background
						// process check.	
						return_val = execvp(user_input.arguments[0], user_input.arguments);
						fflush(stdout);
						if (return_val == 0) break;
						perror("execvp");
						exit(1);
						//break;
				default:
						// if & is specified, prints child process ID, runs process with WNOHANG,
						// and adds the pid to the pid_list for termination at a later time.
						// if no & is specified, waitpid is specified without WNOHANG and wait for child to terminate.
						if (arg_count > 1 && (strncmp(user_input.arguments[arg_count-1], "&", 1) == 0)) {
							printf("Child Process ID %d has begun\n", spawnpid);
							user_input.pid_list[pid_count] = spawnpid;
							pid_count++;
							childPid = waitpid(spawnpid, &childStatus, WNOHANG);
						} else {
							childPid = waitpid(spawnpid, &childStatus, 0);
						}
						break;
				}
			}
		}
		// clears arguments array to prepare for new command
		memset(user_input.arguments, 0, 512);
	}	
	
	// frees memory from arguments array
	for (size_t i = 0; i < arg_count; i++) {
		free(user_input.arguments[i]);
		user_input.arguments = NULL;
	}
	free(user_input.arguments);
	user_input.arguments = NULL;	

	
	return EXIT_SUCCESS;
	
}	
