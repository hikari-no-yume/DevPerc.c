#ifndef INTERPRETER_H
#define INTERPRETER_H

#include <stddef.h>
#include <stdbool.h>

typedef struct {
	const char *text;
	size_t text_len;
	/* this is the start of the line-to-be-executed */
	size_t start_pos;
	unsigned char registers[26];
} interpreter_state;

void init_interpreter(interpreter_state *state, const char *text, size_t text_len);

bool step_interpreter(interpreter_state *state);

#endif
