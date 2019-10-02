/*
 *
 * Copyright 2018-2019 Daniel Dwek
 *
 * This file is part of nullify.
 *
 *  nullify is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  nullify is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with nullify.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "errorcodes.h"

int main(int argc, char **argv)
{
	int i, ret;
	unsigned array[2][10] = { { 0 } };


	printf("Checking whether rand() generates the same results "
		"after specified delay\n");

	srand(1);
	printf("seed=1\n\t");
	for (i=0;i<10;i++) {
		array[0][i] = rand() % 10;
		printf("%u ", array[0][i]);
	}
	printf("\n");

	srand(1);
	printf("seed=1 (delayed execution)\n\t");
	for (i=0;i<5;i++) {
		array[1][i] = rand() % 10;
		printf("%u ", array[0][i]);
	}
	sleep(3);
	for (i=5;i<10;i++) {
		array[1][i] = rand() % 10;
		printf("%u ", array[0][i]);
	}
	printf("\n");

	ret = ERR_PASS;
	for (i=0;i<10;i++) {
		if (array[0][i] != array[1][i]) {
			ret = ERR_FAIL;
			break;
		}
	}

	return ret;
}

