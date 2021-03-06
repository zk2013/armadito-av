/***

Copyright (C) 2015, 2016 Teclib'

This file is part of Armadito core.

Armadito core is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Armadito core is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with Armadito core.  If not, see <http://www.gnu.org/licenses/>.

***/

#include <libjrpc/jrpc.h>

#include "hash.h"

#include <string.h>

struct jrpc_mapper {
	struct hash_table *method_table;
	struct hash_table *error_message_table;
};

struct jrpc_mapper *jrpc_mapper_new(void)
{
	struct jrpc_mapper *mapper = malloc(sizeof(struct jrpc_mapper));

	mapper->method_table = hash_table_new(HASH_KEY_STR, (free_cb_t)free, NULL);
	mapper->error_message_table = hash_table_new(HASH_KEY_INT, NULL, NULL);

	return mapper;
}

int jrpc_mapper_add(struct jrpc_mapper *mapper, const char *method, jrpc_method_t method_fun)
{
	char *p = strdup(method);

	if (!hash_table_insert(mapper->method_table, p, method_fun)) {
		free(p);
		return 1;
	}

	return 0;
}

jrpc_method_t jrpc_mapper_find(struct jrpc_mapper *mapper, const char *method)
{
	return hash_table_search(mapper->method_table, (void *)method);
}

int jrpc_mapper_add_error_message(struct jrpc_mapper *mapper, unsigned char error_code, const char *error_message)
{
	if (!hash_table_insert(mapper->error_message_table, H_INT_TO_POINTER(error_code), (void *)error_message))
		return 1;

	return 0;
}

const char *jrpc_mapper_get_error_message(struct jrpc_mapper *mapper, unsigned char error_code)
{
	return hash_table_search(mapper->error_message_table, H_INT_TO_POINTER(error_code));
}
