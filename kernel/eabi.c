#include "types.h"

uint __udivmodsi4(uint num, uint den, uint * rem_p) {
	uint quot = 0, qbit = 1;

	if (den == 0) {
		return 0;
	}

	/*
	 * left-justify denominator and count shift
	 */
	while ((int) den >= 0) {
		den <<= 1;
		qbit <<= 1;
	}

	while (qbit) {
		if (den <= num) {
			num -= den;
			quot += qbit;
		}
		den >>= 1;
		qbit >>= 1;
	}

	if (rem_p) *rem_p = num;

	return quot;
}


int __aeabi_idiv(int num, int den) {
	int minus = 0;
	int v;

	if (num < 0) {
		num = -num;
		minus = 1;
	}
	if (den < 0) {
		den = -den;
		minus ^= 1;
	}

	v = __udivmodsi4(num, den, 0);
	if (minus) v = -v;

	return v;
}

uint __aeabi_uidiv(uint num, uint den) {
	return __udivmodsi4(num, den, 0);
}

uint __aeabi_uidivmod(uint num, uint den) {
	uint val = __udivmodsi4(num, den, 0);
	return num - val * den;
}