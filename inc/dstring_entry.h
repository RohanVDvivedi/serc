#ifndef DSTRING_ENTRY_H
#define DSTRING_ENTRY_H

#include<dstring.h>

typedef struct dentry dentry;
struct dentry
{
	// cached hashvalue of the key
	unsigned int key_hash_value;

	dstring key;

	dstring value;
};

dentry* get_dentry(const dstring* key, const dstring* value);
dentry* get_dentry_cstrings(const char* key, const char* value);

int key_compare_dentry_CASE_SENSITIVE(const void* entry1, const void* entry2);
int key_compare_dentry_CASE_INSENSITIVE(const void* entry1, const void* entry2);

unsigned int key_hash_dentry(const void* entryp);

void delete_dentry(dentry* entryp);

#endif