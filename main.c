#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/*
  Function Declarations for builtin shell commands:
 */
int lsh_cd(char **args);
int lsh_help(char **args);
int lsh_exit(char **args);

char *builtin_str[] = {
	"cd",
	"help",
	"exit"
};

int (*builtin_func[]) (char **) = {
	&lsh_cd,
	&lsh_help,
	&lsh_exit
};

int lsh_num_builtins() {
  return sizeof(builtin_str) / sizeof(char *);
}

int lsh_cd(char **args)
{
  if (args[1] == NULL) {
    fprintf(stderr, "lsh: expected argument to \"cd\"\n");
  } else {
    if (chdir(args[1]) != 0) {
      perror("lsh");
    }
  }
  return 1;
}

int lsh_help(char **args)
{
  int i;
  printf("Mani's Shell\n");
  printf("Type program names and arguments, and hit enter.\n");
  printf("The following are built in:\n");

  for (i = 0; i < lsh_num_builtins(); i++) {
    printf("  %s\n", builtin_str[i]);
  }

  printf("Use the man command for information on other programs.\n");
  return 1;
}

int lsh_exit(char **args)
{
  return 0;
}

int lsh_launch(char **args)
{
	pid_t pid, wpid;
	int status;

	pid = fork();
	if (pid == 0){
		//Child process

		//Execvp is a variant of exec
		//This expects a program name and a vector(v)
		//The 'p' means that instead of providing a full file path,
		//we simply provide the name and let the OS search for the path
		if (execvp(args[0], args) == -1){
			perror("shell");
		}
		exit(EXIT_FAILURE);
	} else if (pid < 0){
		//Error forking
		perror("shell");
	} else{
		//Parent process
		do {
			//Parent proc waits for the child to finish or exit
			wpid = waitpid(pid, &status, WUNTRACED);
		} while (!WIFEXITED(status) && !WIFSIGNALED(status));
	}

	return 1;
}

int lsh_execute(char **args)
{
	int i;

	if (args[0] == NULL){
		return 1;
	}

	for (i = 0; i < lsh_num_builtins(); i++) {
    	if (strcmp(args[0], builtin_str[i]) == 0){
    		return (*builtin_func[i])(args);
    	}
  	}

  	return lsh_launch(args);
}

#define LSH_TOK_BUFSIZE 64
#define LSH_TOK_DELIM " \t\r\n\a"
char **lsh_split_line(char *line)
{
	int bufsize = LSH_TOK_BUFSIZE, position = 0;
	char **tokens = malloc(sizeof(char*) * bufsize);
	char *token;

	if (!tokens){
		fprintf(stderr, "shell: allocation error\n");
		exit(EXIT_FAILURE);
	}

	token = strtok(line, LSH_TOK_DELIM);
	while (token != NULL){
		tokens[position] = token;
		position++;

		if (position >= bufsize){
			bufsize += LSH_TOK_BUFSIZE;
			tokens = realloc(tokens, bufsize * sizeof(char*));

			if (!tokens){
				fprintf(stderr, "shell: allocation error\n");
				exit(EXIT_FAILURE);
			}
		}

		token = strtok(NULL, LSH_TOK_DELIM);
	}
	tokens[position] = NULL;
	return tokens;
}

#define LSH_RL_BUFSIZE 1024
//Read a line from STDIN
char *lsh_read_line(void)
{
	int bufsize = LSH_RL_BUFSIZE;
	int position = 0;
	char *buffer = malloc(sizeof(char) * bufsize);
	int c;

	if (!buffer){
		fprintf(stderr, "shell: allocation error\n");
		exit(EXIT_FAILURE);
	}

	while(1) {
		//Read a char as an int to check for EOF
		c = getchar();

		//If we reach end of file or new line, return buffer
		if (c == EOF || c == '\n'){
			buffer[position] = '\0';
			return buffer;
		} else{
			buffer[position] = c;
		}
		position++;

		//If buffer runs out of space, expand it
		if (position >= bufsize){
			bufsize += LSH_RL_BUFSIZE;
			buffer = realloc(buffer, bufsize);
			if (!buffer){
				fprintf(stderr, "shell: allocation error\n");
				exit(EXIT_FAILURE);
			}
		}
	}

	//There is also a way to do all this by using the stdio getline function
	//char *line = NULL;
	//ssize_t bufsize = 0;
	//getline(&line, &buffer, stdin);
	//return line;
}

//Main Loop code
void lsh_loop(void)
{
	char *line;
	char **args;
	int status;

	do {
		printf("> ");
		line = lsh_read_line();
		args = lsh_split_line(line);
		status = lsh_execute(args);

		free(line);
		free(args);
	} while(status);
}



int main(int argc, char **argv)
{
	lsh_loop();

	return EXIT_SUCCESS;
}