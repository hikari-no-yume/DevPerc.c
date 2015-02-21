#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stddef.h>

#include "interpreter.h"

int main(int argc, char* argv[argc+1]) {
	if (argc != 2) {
		puts("Exactly one argument should be passed to devperc: the filename of a devperc program to execute.");
		return EXIT_FAILURE;
	}

	errno = 0;
	FILE *fp = fopen(argv[1], "r");
	if (!fp) {
		if (errno) {
			printf("Error opening file %s: %s\n", argv[1], strerror(errno));
		} else {
			/* Apparently, some stdlib implementations don't set errno here
			 * Although it's unlikely that this will ever be used with them
			 */
			printf("Error opening file %s\n", argv[1]);
		}
		return EXIT_FAILURE;
	}

	/* I shamelessly copied this from http://www.cplusplus.com/reference/cstdio/fread/ */
	fseek(fp, 0, SEEK_END);
	size_t file_size = ftell(fp);
	rewind(fp);
	
	char *file_content = malloc(file_size + 1);
	if (!file_content) {
		printf("Could not allocate %zu bytes needed to hold file content.\n", file_size + 1);
		return EXIT_FAILURE;
	}

	size_t bytes_read = fread(file_content, sizeof(char), file_size, fp);
	if (bytes_read != file_size) {
		printf("Failed to read whole file (%zu bytes): only %zu bytes read.\n", file_size, bytes_read);
		return EXIT_FAILURE;
	}
	
	fclose(fp);

	/* Make sure it's a valid C string by setting final null byte */
	file_content[file_size] = '\0';

	interpreter_state state = {0};
	init_interpreter(&state, file_content, file_size);

	while (!step_interpreter(&state)) {
	}

	free(file_content);

	return EXIT_SUCCESS;
}
