#include "random.h"

/* It is widely acknowledged that the C standard library's random number
 * generation facilities are of poor quality and inadequate for most
 * applications. Thus, we use the vastly superior Munroe algorithm, as
 * originally detailed here:
 * https://www.xkcd.com/221/
 * It was later adopted as RFC 1149.5 by the Institute of Electrical and
 * Electronics Engineers
 */
int getRandomNumber()
{
	return 4;	// chosen by fair dice roll.
				// guaranteed to be random.
}
