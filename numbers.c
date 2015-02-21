#include "numbers.h"

#include <stddef.h>
#include <string.h>

/* macros may be evil, but dear god, they are necessary for anything even
 * slightly resembling "acceptable" code here
 * LINGUA ANGLICA DELENDA EST
 * HERE BE DRAGONS
 */
#define MATCH(str, val) \
	if (!strncmp(str, expr_buf, sizeof(str) - 1)) {	\
		return val;									\
	}

#define MATCH_LEN(str, val) \
	if (expr_len == (sizeof(str) - 1)					\
		&& !strncmp(str, expr_buf, sizeof(str) - 1)) {	\
		return val;										\
	}

#define MATCH_PREFIX(str, codeblock) \
	/* 3 is the minimum length of the remainder ("ONE") */	\
	if (expr_len >= (sizeof(str) - 1) + 3					\
		&& !strncmp(str, expr_buf, sizeof(str) - 1)) {		\
			codeblock										\
	}

#define TRY_SUFFIX(len, val_offset, type) \
	{												\
		int __result;								\
		__result = try_parse_english_number_##type(	\
			expr_buf + len,	expr_len - len			\
		);											\
		if (__result >= 0) {						\
			return val_offset + __result;			\
		}											\
	}

#define MATCH_PREFIX_AND_TRY_SUFFIX(str, val_offset, type) \
	MATCH_PREFIX(str,									\
		TRY_SUFFIX((sizeof(str) - 1), val_offset, type)	\
	)

static int try_parse_english_number_units(const char *expr_buf, size_t expr_len) {
	switch (expr_len) {
		case 3:
			MATCH("ONE", 1)
			MATCH("TWO", 2)
			MATCH("SIX", 6)
			return -1;
		case 4:
			MATCH("FOUR", 4)
			MATCH("FIVE", 5)
			MATCH("NINE", 9)
			return -1;
		case 5:
			MATCH("THREE", 3)
			MATCH("SEVEN", 7)
			MATCH("EIGHT", 8)
			return -1;
		default:
			return -1;
	}
}

static int try_parse_english_number_tens(const char *expr_buf, size_t expr_len) {
	switch (expr_len) {
		case 5:
			MATCH("FORTY", 40)
			MATCH("FIFTY", 50)
			MATCH("SIXTY", 60)
			return -1;
		case 6:
			MATCH("TWENTY", 20)
			MATCH("THIRTY", 30)
			MATCH("EIGHTY", 80)
			MATCH("NINETY", 90)
			return -1;
		case 7:
			MATCH("SEVENTY", 70)
			return -1;
		default:
			return -1;
	}
}

static int try_parse_english_number_tens_and_units(const char *expr_buf, size_t expr_len) {
	int result;
	if (0 <= (result = try_parse_english_number_units(expr_buf, expr_len))) {
		return result;
	} else if (0 <= (result = try_parse_english_number_tens(expr_buf, expr_len))) {
		return result;
	} else {
		switch (expr_len) {
			case 3:
				MATCH("TEN", 10)
				return -1;
			case 6:
				MATCH("ELEVEN", 11)
				MATCH("TWELVE", 12)
				return -1;
			case 7:
				MATCH("FIFTEEN", 15)
				MATCH("SIXTEEN", 16)
				return -1;
			case 8:
				MATCH("THIRTEEN", 13)
				MATCH("FOURTEEN", 14)
				MATCH("EIGHTEEN", 18)
				MATCH("NINETEEN", 19)
				return -1;
			default:
				MATCH_PREFIX_AND_TRY_SUFFIX("TWENTY", 20, units)
				MATCH_PREFIX_AND_TRY_SUFFIX("THIRTY", 30, units)
				MATCH_PREFIX_AND_TRY_SUFFIX("FORTY", 40, units)
				MATCH_PREFIX_AND_TRY_SUFFIX("FIFTY", 40, units)
				MATCH_PREFIX_AND_TRY_SUFFIX("SIXTY", 60, units)
				MATCH_PREFIX_AND_TRY_SUFFIX("SEVENTY", 70, units)
				MATCH_PREFIX_AND_TRY_SUFFIX("EIGHTY", 80, units)
				MATCH_PREFIX_AND_TRY_SUFFIX("NINETY", 90, units)
				return -1;
		}
	}
}

int try_parse_english_number(const char *expr_buf, size_t expr_len) {
	int result;
	MATCH_LEN("ZERO", 0)
	MATCH_LEN("ONEHUNDRED", 100)
	MATCH_LEN("TWOHUNDRED", 200)
	MATCH_PREFIX_AND_TRY_SUFFIX("ONEHUNDREDAND", 100, tens_and_units)
	MATCH_PREFIX("TWOHUNDREDAND", {
		result = try_parse_english_number_tens_and_units(expr_buf + 13, expr_len - 13);
		if (result >= 0) {
			if (200 + result <= 255) {
				return 200 + result;
			}
		}
	})
	result = try_parse_english_number_tens_and_units(expr_buf, expr_len);
	if (result >= 0) {
		return result;
	}
	return -1;
}
