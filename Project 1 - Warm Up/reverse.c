// Project 1 - Warm-up
// Lauren Sherburne
// 01/23/2022

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <errno.h>

// create a struct for a linked list
typedef struct node {
    char *value;
    struct node *next;
} node_t;

// add a node and its value to the front of the linked list
static struct node* addToFront(struct node* currentHead, char *line, size_t size) {
	struct node* new = (struct node*) malloc(sizeof(struct node));
	// new->value = malloc(size);
	if (new->value == NULL || new == NULL) {	// check for malloc error
		fprintf(stderr, "error: malloc failed\n");
		exit(EXIT_FAILURE);
	}

	// for (int i = 0; i < size; i++) {
	// 	if (line[i] != '\n') {
	// 		new->value[i] = line[i];
	// 	} else {
	// 		new->value[i] = '\0';
	// 	}
	// }
	new->value = line;
	new->next = currentHead;

	return new;
}

// iterate over the linked list and print each line to the screen
static void printToScreen(struct node* start) {
	while(start->next != NULL) {
		if (start->next->next == NULL) {
			printf("%s", start->value);
		} else {
				printf("%s", start->value);
		}
		start = start->next;
	}

	return;
}

// iterate over the linked list and print each line to the screen
static int printToFile(struct node* start, char *outFileName) {
	// open output file
	FILE *fileOut;
	fileOut = fopen(outFileName, "w");

	// check for error
	if (fileOut == NULL) {
		fprintf(stderr, "error: cannot open file \'%s\'\n", outFileName);
		return 1;
	}

		while(start->next != NULL) {
		if (start->next->next == NULL) {
			fprintf(fileOut, "%s", start->value);
		} else {
			fprintf(fileOut, "%s", start->value);
		}
		start = start->next;
	}

	return 0;
}

// free the space allocated for the linked list
static void freeSpace(struct node* head) {
	while(head->next != NULL) {
		free(head->value);
		struct node* tempNode = head;
		head = head->next;
		free (tempNode);
	}
}




int main(int argc, char *argv[]) {
	// check for incorrect number of arguments
	if (!(argc == 2 || argc == 3)) {
		fprintf(stderr, "usage: reverse <input> <output>\n");
		exit(EXIT_FAILURE);
	}

	// check for same input and output files
	if (argc == 3) {
		for (int i = 0; i < sizeof(argv[1]); i++) {
			if (argv[1][i] != argv[2][i]) {
				break;
			}
			if (i == sizeof(argv[1]) - 1 && argv[1][i] == argv[2][i]) {
				fprintf(stderr, "error: input and output file must differ\n");
				exit(EXIT_FAILURE);
			}
		}
	}

	// create a head node for the linked list
	struct node* head = (struct node*) malloc(sizeof(struct node));
	if (head == NULL) {	// check for malloc error
		fprintf(stderr, "error: malloc failed\n");
		exit(EXIT_FAILURE);
	}
	head->next = NULL;

	// define file variables
	FILE *fileIn;
	char *line = NULL;
	size_t len = 0;
	ssize_t nread;

	// open file stream (read only) using filename argument
	fileIn = fopen(argv[1], "r");

	// check for error
	if (fileIn == NULL) {
		fprintf(stderr, "error: cannot open file \'%s\'\n", argv[1]);
		exit(EXIT_FAILURE);
	}

	// read from file
	while ((nread = getline(&line, &len, fileIn)) != -1) {
		head = addToFront(head, line, len);
		line = NULL;
	}

	// free variables
	free(line);

	// iterate over the linked list and print the lines
	if (argc == 3) {
		int result = printToFile(head, argv[2]);
		if (result == 1) {
			exit(EXIT_FAILURE);
		}
	} else {
		printToScreen(head);
	}
	freeSpace(head);

	// close file and exit successfully
	fclose(fileIn);	
	exit(EXIT_SUCCESS);
}
