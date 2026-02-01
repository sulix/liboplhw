/*
 * oplhw: ALSA hwdep-based library for OPL2-based soundcards.
 *
 * Copyright (C) 2022 by David Gow <david@davidgow.net>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "oplhw.h"

// A quick tool to list any oplhw-compatible devices detected on the system.
int main(int argc, char **argv)
{
	struct oplhw_devlist *list, *entry;
	int path_width, path_max_width = 4; /* Length of "Path" */
	printf("%s: List OPL2-compatible synthesisers\n", argv[0]);
	printf("(C) 2026 David Gow\n");
	printf("\n");

	list = oplhw_Enumerate();

	if (!list->path)
	{
		printf("No devices found.\n");
		return -1;
	}

	/* Get the maximum width of the path. */
	for (entry = list; entry; entry = entry->next)
	{
		path_width = strlen(entry->path);
		if (path_width > path_max_width)
			path_max_width = path_width;
	}


	printf("%-*s | Description\n", path_max_width, "Path");
	printf("%*s |\n", path_max_width, "");
	for (entry = list; entry; entry = entry->next)
	{
		printf("%-*s | %s\n", path_max_width, entry->path, entry->description);
	}
	printf("\n");
	return 0;
}
