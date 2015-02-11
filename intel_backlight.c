/*
 * Copyright © 2011 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *	Chris Wilson <chris@chris-wilson.co.uk>
 *	Maurizio Vairani <maurizio.vairani@cloverinformatica.it>
 */

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>


#include "intel_gpu_tools.h"
#define NUM_ELEMENTS(array) (sizeof(array) / sizeof(array[0]))

/* XXX PCH only today */

static uint32_t reg_read(uint32_t reg)
{
	return *(volatile uint32_t *)((volatile char*)mmio + reg);
}

static void reg_write(uint32_t reg, uint32_t val)
{
	*(volatile uint32_t *)((volatile char*)mmio + reg) = val;
}

static int const brightness_levels[] = {1, 2, 4, 6, 9, 12, 16, 20, 25, 30, 36,
										43, 51, 60, 70, 80, 90, 100};

static int brightness_incr(int curr)
{
	int i;
	for (i = 0; i < NUM_ELEMENTS(brightness_levels) - 1; ++i)
		if (curr < brightness_levels[i])
			break;
	return brightness_levels[i];
}

static int brightness_decr(int curr)
{
	int i;
	for (i = NUM_ELEMENTS(brightness_levels) - 1; i > 0; --i)
		if (brightness_levels[i] < curr)
			break;
	return brightness_levels[i];
}

void usage(void)
{
	printf("Usage: intel_backlight [-n] [-r <register value> | "
		"<brightness level>]\n");
	printf("\t-n show only brightness percent level as number. Useful in "
		"scripts,\n\t   eg. set BACKLIGHT=`intel_backlight -n incr`.\n");
	printf("\t-r <register value> set the brightness hardware register to "
		"value.\n\t   Warning with -r0 the LCD is black.\n");
	printf("\t<brightness level> may be a percentage value, the string "
		"\"incr\" or the\n\t   string \"decr\" to set, increment or decrement "
		"the bringhtness level.\n"); 
}

int parse_long(char *in_str, long *result)
{
	char *endptr, *str;
	long val;
	str = in_str;
	errno = 0;    /* To distinguish success/failure after call */
	val = strtol(str, &endptr, 10);

	/* Check for various possible errors */
	if ((errno == ERANGE && (val == LONG_MAX || val == LONG_MIN))
		|| (errno != 0 && val == 0)) {
		perror("strtol");
		return EXIT_FAILURE;
	}
	if (endptr == str) {
		fprintf(stderr, "No digits were found\n");
		return EXIT_FAILURE;
	}
	*result = val;
	return EXIT_SUCCESS;
}

int main(int argc, char** argv)
{
	uint32_t current, reg = UINT32_MAX, max;
	int result, quiet = 0, ch;

	intel_get_mmio(intel_get_pci_device());
	current = reg_read(BLC_PWM_CPU_CTL) & BACKLIGHT_DUTY_CYCLE_MASK;
	max = reg_read(BLC_PWM_PCH_CTL2) >> 16;
	if (max == 0) {
		fprintf(stderr, "Backlight is unsupported (%d/%d).\n", current, max);
		return EXIT_FAILURE;
	}

	while ((ch = getopt(argc, argv, "nr:")) != -1) {
		long r;
		switch (ch) {
		case 'r':
			if (parse_long(optarg, &r) == EXIT_FAILURE) {
				fprintf(stderr, "Invalid register value: not a number.\n");
				usage();
				return EXIT_FAILURE;
			}
			if (r < 0) {
				fprintf(stderr, "Invalid register value: %ld is negative.\n",
					r);
				usage();
				return EXIT_FAILURE;
			}
			else if (r > max) {
				fprintf(stderr, "Invalid register value: %ld is above %d, the "
					"maximum allowed value.\n", r, max);
				usage();
				return EXIT_FAILURE;
			}
			reg = (uint32_t) r;
			break;
		case 'n':
			quiet = 1;
			break;
		case '?':
		default:
			usage();
			return EXIT_FAILURE;
		}
	}
	argc -= optind;
	argv += optind;
	if (argc > 1 || (argc == 1 && reg != UINT32_MAX)) {
		usage();
		return EXIT_FAILURE;
	}

	result = 0.5 + current * 100.0 / max;
	if (!quiet)
		printf ("Current backlight value is %d%% (%d/%d).\n", result,
			current, max);
	else if (!(argc == 1 || reg != UINT32_MAX))
		printf("%d\n", result);

	if (argc == 1 || reg != UINT32_MAX) {
		uint32_t val;
		if (argc == 1) {
			uint32_t min;
			if (strcmp(argv[0], "incr") == 0) 
				val = 0.5 + brightness_incr(result) * max / 100.0;
			else if (strcmp(argv[0], "decr") == 0)
				val = 0.5 + brightness_decr(result) * max / 100.0;
			else {
				long lev;
				if (parse_long(argv[0], &lev) == EXIT_FAILURE || lev < 0
					|| lev > 100) {
					fprintf(stderr, "Invalid brightness percentage "
						"level: %s\n", argv[0]);
					usage();
					return EXIT_FAILURE;
				}
				if (lev < 0 || lev >100) {
					fprintf(stderr, "Invalid brightness level: not a "
						"number.\n");
					usage();
					return EXIT_FAILURE;
				}
				
				val = 0.5 + lev * max / 100.0;
			}
			min = 0.5 + 0.5 * max / 100.0;	// 0.5%
			if (val < min)
				val = min;
			else if (val > max)
				val = max;
		}
		else
			val = reg;
		if (val != current) {
			reg_write(BLC_PWM_CPU_CTL, (reg_read(BLC_PWM_CPU_CTL) 
				&~ BACKLIGHT_DUTY_CYCLE_MASK) | val);
			(void) reg_read(BLC_PWM_CPU_CTL);
		}
		result = 0.5 + val * 100.0 / max;
		if (quiet)
			printf("%d\n", result);
		else
			printf ("Set backlight to %d%% (%d/%d).\n", result, val, max);
	}
	return EXIT_SUCCESS;
}
