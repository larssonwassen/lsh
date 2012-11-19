#include <stdio.h>
#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include "parse.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#define WRITE (1)
#define READ (0)
#define PATHS (6)

/*
 * Prototypes
 */

void PrintCommand(int, Command *);
void PrintPgm(Pgm *);
void stripwhite(char *);
void child(int, Command *, Pgm *);
char* findPath(char *);

/* When non-zero, this global means the user is done using this program. */
int done = 0;
char* path[6] = { "/bin/", "/sbin/", "/usr/bin/", "/usr/sbin/",
		"/usr/local/sbin/", "/usr/local/bin/" };
/*
 * Name: main
 *
 * Desc: Gets the ball rolling...
 *
 */
int main(void) {
	Command cmd;
	int n;

	while (!done) {

		char *line;

		line = readline("@ ");

		if (!line) {
			/* Encountered EOF at top level */
			done = 1;
		} else {
			/*
			 * Remove leading and trailing whitespace from the line
			 * Then, if there is anything left, add it to the history list
			 * and execute it.
			 */
			stripwhite(line);

			if (*line) {
				add_history(line);
				/* execute it */
				n = parse(line, &cmd);

				child(0, &cmd, (&cmd)->pgm);

				//PrintCommand(n, &cmd);
			}
		}

		if (line)
			free(line);
	}
	return 0;
}

void child(int fd, Command *cmd, Pgm *p) {
	char *pat;
	int oldPid = getpid();
	pid_t childPid;
	int rv;
	int fds[2];
	int rstdout;
	int rstdin;

	pat = findPath(*(p->pgmlist));
	if (pat == NULL) {
		fprintf(stderr, "The command %s is not found.\n", *(p->pgmlist));
		return;
	}
	fprintf(stderr, "Entering Child() with command %s (%d)\n", pat, oldPid);

	if (pipe(fds) == -1) {
		fprintf(stderr, "Pipe error. Exiting.\n");
		exit(1);
	}

	if ((childPid = fork()) == -1) {
		fprintf(stderr, "Fork error. Exiting.\n");
		exit(1);
	}

	if (childPid) {
		/* Parrent */


		close(fds[READ]);
		close(fds[WRITE]);
		fprintf(stderr,
				"In parrent for command %s(%d) waiting for (%d) to die. \n",
				pat, oldPid, childPid);
		if (!((fd == 0) && ((cmd->bakground) == 1))) 
		{
			wait(&rv);	
		}
			

		fprintf(stderr, "%s(%d) exited with error code %d \n", pat, childPid,
				rv);

	} else {
		/* Child */
		fprintf(stderr, "In Child for command %s(%d)\n", pat, getpid());
		if (fd) {
			fprintf(stderr, "fd is set, setting stdout to fd. Program %s \n",
					pat);
			dup2(fd, WRITE);
		}

		else if((cmd->rstdout) != NULL)
		{
			if((rstdout = open(cmd->rstdout,O_WRONLY|O_CREAT|O_TRUNC,0660)) == -1)
			{
				fprintf(stderr, "The file %s is not found.\n", cmd->rstdin);
				return;
			}
			
			dup2(rstdout,WRITE);
			close(rstdout);

		}
		if ((p->next) != NULL) {
			fprintf(stderr,
					"More programs to run, setting stdin to readEnd of pipe. Program %s \n",
					pat);

			dup2(fds[READ], READ);
			child(fds[WRITE],cmd, p->next);
			close(fds[WRITE]);
		} else {
			if((cmd->rstdin) != NULL)
			{	
				if((rstdin = open(cmd->rstdin,O_RDONLY)) == -1)
				{
					fprintf(stderr, "The file %s is not found.\n", cmd->rstdin);
					return;
				}
			
				dup2(rstdin,READ);
				close(rstdin);
			}
			close(fds[READ]);
		}

		pat = findPath(*(p->pgmlist));
		if (pat == NULL) {
			fprintf(stderr, "The command %s is not found.\n", *(p->pgmlist));
			return;
		}
		fprintf(stderr, "Executing program %s \n", pat);
		if (execve(pat, p->pgmlist, NULL) == -1) {
			fprintf(stderr, "execl Error!");
			exit(1);
		}
		fprintf(stderr, "Done executing program %s \n", pat);

	}
}

char* findPath(char* cmd) {
	if (cmd[0] == '/') {
		return cmd;
	}
	static char buf[255];
	struct stat stats;
	int i;
	for (i = 0; i < PATHS; i++) {
		strcpy(buf, path[i]);
		strcat(buf, cmd);
		if (stat(buf, &stats) == 0) {
			return buf;
		}
	}
	return NULL;
}

/*
 * Name: PrintCommand
 *
 * Desc: Prints a Command structure as returned by parse on stdout.
 *
 */
void PrintCommand(int n, Command *cmd) {
	printf("Parse returned %d:\n", n);
	printf("   stdin : %s\n", cmd->rstdin ? cmd->rstdin : "<none>");
	printf("   stdout: %s\n", cmd->rstdout ? cmd->rstdout : "<none>");
	printf("   bg    : %s\n", cmd->bakground ? "yes" : "no");
	PrintPgm(cmd->pgm);
}

/*
 * Name: PrintPgm
 *
 * Desc: Prints a list of Pgm:s
 *
 */
void PrintPgm(Pgm *p) {
	if (p == NULL)
		return;
	else {
		char **pl = p->pgmlist;

		/* The list is in reversed order so print
		 * it reversed so get right :-)
		 */
		PrintPgm(p->next);
		printf("    [");
		while (*pl)
			printf("%s, ", *pl++);
		printf("]\n");
	}
}

/*
 * Name: stripwhite
 *
 * Desc: Strip whitespace from the start and end of STRING.
 */
void stripwhite(char *string) {
	register int i = 0;
	while (whitespace(string[i]))
		i++;
	if (i)
		strcpy(string, string + i);

	i = strlen(string) - 1;
	while (i > 0 && whitespace(string[i]))
		i--;
	string[++i] = '\0';
}

