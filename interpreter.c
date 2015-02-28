#include "interpreter.h"

#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "numbers.h"

void init_interpreter(interpreter_state *state, const char *text, size_t text_len) {
	state->text = text;
	state->text_len = text_len;
	state->start_pos = 0;

	/* by default registers A-Z are initialised with the ASCII values of A-Z
	 * we're assuming alphabetically-ordered, contiguous letters
	 * but we're assuming ASCII anyway
	 * sorry, IBM
	 */
	for (size_t i = 0; i < 26; i++) {
		state->registers[i] = (unsigned char)('A' + i);
	}
}

static inline unsigned char* lookup_register(const interpreter_state *state, char register_name) {
	if (!('A' <= register_name && register_name <= 'Z')) {
		printf("Error at character %zu: Invalid register name: '%c'.\n", state->start_pos, register_name);
		exit(EXIT_FAILURE);
	}

	return (unsigned char*) &state->registers[register_name - 'A'];
}

static unsigned char interpret_expression(const interpreter_state *state, const char *expr_buf, size_t expr_len) {
	/* register name! */
	if (expr_len == 1) {
		return *lookup_register(state, expr_buf[0]);
	} else {
		int result = try_parse_english_number(expr_buf, expr_len);
		if (result < 0) {
			printf("Unhandled expression: \"%.*s\" (%zu).\n", (int)expr_len, expr_buf, expr_len);
			exit(EXIT_FAILURE);	
		}
		return result;
	}
}

/* given the restrictions on DevPerc syntax, an uncommented line won't be this
 * long
 */
#define BUF_SIZE 1024

/* execute one line
 * this assumes the interpreter is in a valid state and may segfault if not */
bool step_interpreter(interpreter_state *state) {
	/* per the DevPerc "spec" at http://esolangs.org/wiki/Deviating_Percolator
	 * we must read and substitute character-by-character until hitting \n
	 */
	char line_buf[BUF_SIZE + 1];
	size_t pos = state->start_pos;
	size_t line_pos = 0;
	while (true) {
		char in_char = state->text[pos];
		char out_char = in_char;

		/* needs substituting */
		if ('A' <= in_char && in_char <= 'Z') {
			out_char = (char)state->registers[in_char - 'A'];
		}
		
		/* we're done! */
		if (out_char == '\n') {
			line_buf[line_pos] = '\0';
			break;
		}
		
		line_buf[line_pos] = out_char;

		pos++;
		line_pos++;

		if (!(pos < state->text_len)) {
			line_buf[line_pos - 1] = '\0';
			break;
		}

		if (!(pos < BUF_SIZE)) {
			printf("Error at character %zu: line exceeds %zu characters. Increase the BUF_SIZE constant when compiling.\n", pos, (size_t)BUF_SIZE);
			exit(EXIT_FAILURE);
		}
	}

	size_t line_len = line_pos;
	/* reset the position now that we need to parse this line */
	pos = state->start_pos;
	line_pos = 0;

	if (!line_len) {
		printf("Error at character %zu: line cannot be empty.", pos);
		exit(EXIT_FAILURE);
	}

	/* seek out comment, if any, so we can ignore line beyond that point */
	const char *slash_pos = strchr(line_buf, '/');
	size_t statement_len = line_len;
	if (slash_pos) {
		statement_len = slash_pos - (const char *)&line_buf;
	}

	/* seek out space after first word of statement */
	const char *space_pos = strchr(line_buf, ' ');
	if (!space_pos) {
		printf("Error at character %zu: invalid statement \"%.*s\".\n", pos, (int)statement_len, line_buf);
		exit(EXIT_FAILURE);
	}
	const size_t name_len = space_pos - (const char *)&line_buf;

	/* PUT statement */
	if (name_len == 3 && !strncmp("PUT", line_buf, 3)) {
		/* line after space must be an expression */
		unsigned char result = interpret_expression(state, space_pos + 1, statement_len - 4);
		putchar(result);
	/* GET statement */
	} else if (name_len == 3 && !strncmp("GET", line_buf, 3)) {
		/* line after space must be an expression (register name) */
		char register_name = (char)interpret_expression(state, space_pos + 1, statement_len - 4);
		
		*lookup_register(state, register_name) = getchar();
	/* DEFINE TO statement */
	} else if (name_len == 6 && !strncmp("DEFINE", line_buf, 6)) {
		/* line after space should be "<expr> TO <expr>" */
		const char *to_pos = strstr(space_pos + 1, " TO ");

		/* strpos will look beyond end of statement as it's not bound by length
		 * so we should check in case there was a comment like "/ TO ", say
		 */
		if (!to_pos || to_pos > line_buf + statement_len) {
			printf("Error at character %zu: invalid DEFINE TO statement.\n", pos); 
			exit(EXIT_FAILURE);
		}

		/* now to just interpret our two expressions! */
		char register_name = (char)interpret_expression(
			state,
			/* name is between "DEFINE " and " TO " */
			space_pos + 1,
			to_pos - (space_pos + 1)
		);
		unsigned char register_value = interpret_expression(
			state,
			/* new value is between " TO " and end of statement */
			to_pos + 4,
			(statement_len - (to_pos + 4 - (const char*)&line_buf))
		);

		*lookup_register(state, register_name) = register_value;
	} else {
		printf("Error at character %zu: unrecognised statement name: \"%.*s\".\n", pos, (int)name_len, line_buf); 
		exit(EXIT_FAILURE);
	}

	/* all's good: proceed to next line */
	state->start_pos = pos + line_len + 1;

	/* EOF */
	if (!(state->start_pos < state->text_len)) {
		/* done */
		return 1;
	} else {
		/* not done */
		return 0;
	}
}
