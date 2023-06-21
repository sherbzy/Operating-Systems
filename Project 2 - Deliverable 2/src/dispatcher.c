#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dispatcher.h"
#include "shell_builtins.h"
#include "parser.h"

#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <wait.h>
#include <sys/stat.h>
#include <fcntl.h>

/**
 * dispatch_external_command() - run a pipeline of commands
 *
 * @pipeline:   A "struct command" pointer representing one or more
 *              commands chained together in a pipeline.  See the
 *              documentation in parser.h for the layout of this data
 *              structure.  It is also recommended that you use the
 *              "parseview" demo program included in this project to
 *              observe the layout of this structure for a variety of
 *              inputs.
 *
 * Note: this function should not return until all commands in the
 * pipeline have completed their execution.
 *
 * Return: The return status of the last command executed in the
 * pipeline.
 */
static int dispatch_external_command(struct command *pipeline)
{
	// execute a single command...
	if (pipeline->pipe_to == NULL) {
		// copy argv to save first command
		// int iterator = 0;
		// char *copyArgv[256] = malloc(sizeof(pipeline->argv) * sizeof(char));
		// while (pipeline->argv[iterator] != NULL) {
		// 	copyArgv[iterator] = pipeline->argv[iterator];
		// }

		// fork to create a child process
		pid_t resultFork = fork();

		// check for fork error
		if (resultFork == -1) {
			fprintf(stderr, "Unable to fork: %s\n",
				strerror(errno));
			return -1;
		}

		// use exit in the child or if system... failed
		if (resultFork == 0) { // child process
			// search for and run the command
			int resultExvp = execvp(pipeline->argv[0], pipeline->argv);

			// check for error and exit
			if (resultExvp == -1) {
				fprintf(stderr, "Unable to execute command in child process: %s\n", strerror(errno));
				exit(-1);
			} else {
				exit(0);
			}

		} else if (resultFork > 0) { // parent process
			// wait for child to continue running
			int status;
			waitpid(resultFork, &status, 0);

			if (WIFEXITED(status)) {
				// child terminated normally
				// fprintf(stderr, "Child process executed and terminated normally (Return code: %d)\n", WIFEXITED(status));
				return WEXITSTATUS(status);
			} else if (WIFSTOPPED(status)) {
				// returns signal of what caused the child to stop
				fprintf(stderr, "Child process incomplete: %d\n", WIFSTOPPED(status));
				return WEXITSTATUS(status);
			}
		} else { // pid < 0, error
			fprintf(stderr, "Unable to fork: %s\n", strerror(errno));
		}


	} else { // execute all commands in the pipeline

		int prev_pipe[2] = {STDIN_FILENO, STDOUT_FILENO};
		int current_pipe[2] = {STDIN_FILENO, STDOUT_FILENO};

		int exitStatus;

		// bool to check for first command
		bool isFirstCommand = true;

		while (pipeline != NULL) { // loop through each command
			// input, output
			int input, output;

			// determine the output type
			switch (pipeline->output_type) {
			case COMMAND_OUTPUT_STDOUT:
				output = STDOUT_FILENO;
				break;
			case COMMAND_OUTPUT_FILE_APPEND:
				output = open(pipeline->output_filename, O_APPEND | O_WRONLY | O_CREAT);
				break;
			case COMMAND_OUTPUT_FILE_TRUNCATE:
				output = open(pipeline->output_filename, O_TRUNC | O_WRONLY | O_CREAT);
				break;
			case COMMAND_OUTPUT_PIPE:
				pipe(current_pipe);
				output = current_pipe[1];
				input = current_pipe[0];
				break;
			default:	// the command output type is not one of the possible options
				fprintf(stderr, "Unable to determine output type: %u\n", pipeline->output_type);
			}

			// determine the input type
			if (pipeline->input_filename == NULL) {	// no input file
				// first command gets input from stdin
				if (isFirstCommand) {
					input = STDIN_FILENO;
					isFirstCommand = false;
				} else {	// other commands input from the previous pipe
					input = prev_pipe[0];
				}
			} else {	// input from file
				input = open(pipeline->input_filename, O_RDONLY);
			}
			
			// set the current pipe's input and output
			current_pipe[1] = output;


			// fork()
			// fork to create a child process
			pid_t resultFork = fork();

			// check for fork error
			if (resultFork == -1) {
			fprintf(stderr, "Unable to fork: %s\n",
				strerror(errno));
			return -1;
			}


			if (resultFork == 0) {	// if child:
				// dup2 if the input is not stdin
				if (input != STDIN_FILENO) {
					// dup
					if (dup2(input, STDIN_FILENO) == -1) {
						fprintf(stderr, "Failure using dup2(input, STDIN_FILENO): %s\n", strerror(errno));
					}
				}

				// dup2 if the output is not stdout
				if (output != STDOUT_FILENO) {
					// dup
					if (dup2(output, STDOUT_FILENO) == -1) {
						fprintf(stderr, "Failure using dup2(output, STDOUT_FILENO): %s\n", strerror(errno));
					}
					
				}

				// close the current input/output pipes as necessary
				if (input != 0 && input != 1 && input != 2) {	// input check
					if (close(current_pipe[0]) == -1) {
						fprintf(stderr, "Failure closing current_pipe[0] (%d): %s\n", current_pipe[0], strerror(errno));
					}
				}
				

				// 	exec()
				// search for and run the command
				int resultExvp = execvp(pipeline->argv[0], pipeline->argv);

				// check for error and exit
				if (resultExvp == -1) {
					fprintf(stderr, "Unable to execute command in child process: %s\n", strerror(errno));
					exit(-1);
				} else {
					exit(0);
				}

			} else if (resultFork > 0) {	// if parent:
				if (output != 0 && output != 1 && output != 2) {	// output check, close read
					if (close(current_pipe[1]) == -1) {
						fprintf(stderr, "Failure closing current_pipe[0] (%d): %s\n", current_pipe[0], strerror(errno));
					}
				}

				// 	wait for child
				int status;
				waitpid(resultFork, &status, 0);

				if (WIFEXITED(status)) {
					// child terminated normally
					// fprintf(stderr, "Child process executed and terminated normally (Return code: %d)\n", WIFEXITED(status));
					exitStatus = WEXITSTATUS(status);
				} else if (WIFSTOPPED(status)) {
					// returns signal of what caused the child to stop
					fprintf(stderr, "Child process incomplete: %d\n", WIFSTOPPED(status));
					return WEXITSTATUS(status);
				}
			} else { // pid < 0, error
				fprintf(stderr, "Unable to fork: %s\n", strerror(errno));
			}

			// prev_pipe = current_pipe
			prev_pipe[0] = current_pipe[0];
			prev_pipe[1] = current_pipe[1];

			// move to next command
			pipeline = pipeline->pipe_to;
		}

		return exitStatus;
	}

	// exit...something might be broken
	return -1;
}

/**
 * dispatch_parsed_command() - run a command after it has been parsed
 *
 * @cmd:                The parsed command.
 * @last_rv:            The return code of the previously executed
 *                      command.
 * @shell_should_exit:  Output parameter which is set to true when the
 *                      shell is intended to exit.
 *
 * Return: the return status of the command.
 */
static int dispatch_parsed_command(struct command *cmd, int last_rv,
				   bool *shell_should_exit)
{
	/* First, try to see if it's a builtin. */
	for (size_t i = 0; builtin_commands[i].name; i++) {
		if (!strcmp(builtin_commands[i].name, cmd->argv[0])) {
			/* We found a match!  Run it. */
			return builtin_commands[i].handler(
				(const char *const *)cmd->argv, last_rv,
				shell_should_exit);
		}
	}

	/* Otherwise, it's an external command. */
	return dispatch_external_command(cmd);
}

int shell_command_dispatcher(const char *input, int last_rv,
			     bool *shell_should_exit)
{
	int rv;
	struct command *parse_result;
	enum parse_error parse_error = parse_input(input, &parse_result);

	if (parse_error) {
		fprintf(stderr, "Input parse error: %s\n",
			parse_error_str[parse_error]);
		return -1;
	}

	/* Empty line */
	if (!parse_result)
		return last_rv;

	rv = dispatch_parsed_command(parse_result, last_rv, shell_should_exit);
	free_parse_result(parse_result);
	return rv;
}

	// checks if first command
	// checks if last command
