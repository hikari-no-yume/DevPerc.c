#include "interpreter.h"

#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

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

/* substitutes character in the input file with a register value */
static inline char substitute(const interpreter_state *state, char in_char) {
	/* needs substituting */
	if ('A' <= in_char && in_char <= 'Z') {
		return (char)state->registers[in_char - 'A'];
	} else {
		return in_char;
	}
}

static size_t pos_to_line_no(const interpreter_state *state, size_t pos) {
	size_t line = 0;
	for (size_t i = 0; i < pos && i < state->text_len; i++) {
		if (substitute(state, state->text[i]) == '\n') {
			line++;
		}
	}
	return line;
}

static void error_line(const interpreter_state *state, const char message[static 1], ...) {
	va_list args;
	va_start(args, message);

	fprintf(stderr, "Error at line %zu: ", pos_to_line_no(state, state->start_pos));
	vfprintf(stderr, message, args);
	fprintf(stderr, ".\n");
	exit(EXIT_FAILURE);
}

static void error_line_ex(const interpreter_state *state, size_t pos, const char message[static 1], ...) {
	va_list args;
	va_start(args, message);

	fprintf(stderr, "Error at line %zu: ", pos_to_line_no(state, pos));
	vfprintf(stderr, message, args);
	fprintf(stderr, ".\n");
	exit(EXIT_FAILURE);
}

static inline unsigned char* lookup_register(const interpreter_state *state, char register_name) {
	if (!('A' <= register_name && register_name <= 'Z')) {
		error_line(state, "Invalid register name: '%c'", register_name);
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
			error_line(state, "Unhandled expression: \"%.*s\"", (int)expr_len, expr_buf);
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
		char out_char = substitute(state, in_char);

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
			error_line_ex(state, pos, "line exceeds %zu characters. Increase the BUF_SIZE constant when compiling", (size_t)BUF_SIZE);
		}
	}

	size_t line_len = line_pos;
	/* reset the position now that we need to parse this line */
	pos = state->start_pos;
	line_pos = 0;

	if (!line_len) {
		error_line_ex(state, pos, "line cannot be empty");
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
		error_line_ex(state, pos, "invalid statement \"%.*s\"", (int)statement_len, line_buf);
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
			error_line_ex(state, pos, "invalid DEFINE TO statement"); 
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
		error_line_ex(state, pos, "unrecognised statement name: \"%.*s\"", (int)name_len, line_buf); 
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
