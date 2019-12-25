#include<file_handler.h>

typedef struct file_cache_component file_cache_component;
struct file_cache_component
{
	dstring* file_content;
	rwlock* file_content_rwlock;
};

file_cache_component* get_file_cache_component()
{
	file_cache_component* fcc_component_p = (file_cache_component*)malloc(sizeof(file_cache_component));
	fcc_component_p->file_content = get_dstring("", 10);
	fcc_component_p->file_content_rwlock = get_rwlock();
	return fcc_component_p;
}

void delete_file_cache_component(file_cache_component* fcc_component_p)
{
	delete_dstring(fcc_component_p->file_content);
	delete_rwlock(fcc_component_p->file_content_rwlock);
	free(fcc_component_p);
}

file_content_cache* get_file_content_cache()
{
	file_content_cache* fcc_p = (file_content_cache*) malloc(sizeof(file_content_cache));
	fcc_p->file_content_cache_hashmap = get_hashmap(30, (unsigned long long int (*)(const void*))getHashValueDstring, (int (*)(const void*, const void*))compare_dstring, ELEMENTS_AS_RED_BLACK_BST);
	fcc_p->file_content_cache_hashmap_rwlock = get_rwlock();
	return fcc_p;
}

int read_file_in_dstring(dstring* file_contents_result, file_content_cache* fcc_p, dstring* file_path)
{
	// this is the variable where we will find or make the dstring to store in cache or return
	file_cache_component* file_content_from_cache = NULL;

	// find the file from the cache, once the file has been requested from the server it gets cached, in page cache
	read_lock(fcc_p->file_content_cache_hashmap_rwlock);
	file_content_from_cache = (file_cache_component*)(find_value_from_hash(fcc_p->file_content_cache_hashmap, file_path));
	// if cache is hit, we read from cache itself
	if(file_content_from_cache != NULL)
	{
		read_lock(file_content_from_cache->file_content_rwlock);
		read_unlock(fcc_p->file_content_cache_hashmap_rwlock);
		concatenate_dstring(file_contents_result, file_content_from_cache->file_content);
		read_unlock(file_content_from_cache->file_content_rwlock);
		return 0;
	}
	else
	{
		read_unlock(fcc_p->file_content_cache_hashmap_rwlock);
	}

	int file_content_from_cache_is_locked_by_this_thread = 0;

	// write newly built file cache component to file cache
	write_lock(fcc_p->file_content_cache_hashmap_rwlock);
	file_content_from_cache = (file_cache_component*)(find_value_from_hash(fcc_p->file_content_cache_hashmap, file_path));
	if(file_content_from_cache == NULL)
	{
		// insert the cache entry, so we do not miss the cache next time
		file_content_from_cache = get_file_cache_component();
		insert_entry_in_hash(fcc_p->file_content_cache_hashmap, get_dstring(file_path->cstring ,10), file_content_from_cache);
		write_lock(file_content_from_cache->file_content_rwlock);
		file_content_from_cache_is_locked_by_this_thread = 1;
	}
	write_unlock(fcc_p->file_content_cache_hashmap_rwlock);

	// if we cache miss, we need to go to disk, we go to disk only if we confirm that the file is present
	if(access(file_path->cstring, R_OK) != -1 && file_content_from_cache_is_locked_by_this_thread == 1)
	{	
    	// open the file
    	FILE* file = fopen(file_path->cstring, "rb");

    	// buffer to read the new file, for which we missed the cache
    	#define FILE_READ_BUFFER_SIZE 4000
    	char file_buffer[FILE_READ_BUFFER_SIZE];

    	// loop- through the file, to put it in cache 
		while(!feof(file))
		{
			long long int read_count = fread(file_buffer, sizeof(char), FILE_READ_BUFFER_SIZE, file);
			if(read_count >= 0)
			{
				appendn_to_dstring(file_content_from_cache->file_content, file_buffer, read_count);
			}
		}

		fclose(file);
	}

	// the person who received the lock will only release it
	if(file_content_from_cache_is_locked_by_this_thread == 1)
	{
		write_unlock(file_content_from_cache->file_content_rwlock);
	}

	// if cache has been updated, we read from the cache variable
	if(file_content_from_cache != NULL)
	{
		read_lock(file_content_from_cache->file_content_rwlock);
		concatenate_dstring(file_contents_result, file_content_from_cache->file_content);
		read_unlock(file_content_from_cache->file_content_rwlock);
		return 0;
	}
	return -1;
}

void remove_file_cache_component(const dstring* key, const file_cache_component* value, const void* addpar)
{
	delete_dstring(key);
	delete_file_cache_component(value);
}

void clear_file_content_cache(file_content_cache* fcc_p)
{
	write_lock(fcc_p->file_content_cache_hashmap_rwlock);
	for_each_entry(fcc_p->file_content_cache_hashmap, (void (*)(const void*, const void*, const void*))(remove_file_cache_component), NULL);
	write_unlock(fcc_p->file_content_cache_hashmap_rwlock);
}

void delete_file_content_cache(file_content_cache* fcc_p)
{
	clear_file_content_cache(fcc_p);
	delete_hashmap(fcc_p->file_content_cache_hashmap);
	delete_rwlock(fcc_p->file_content_cache_hashmap_rwlock);
	free(fcc_p);
}

void get_extension_from_file_path(dstring* extension_result, dstring* path)
{
	char* path_t = path->cstring;
	int in_extension = 0;
	while((*(path_t)) != '\0')
	{
		if(in_extension == 1)
		{
			char temp[2] = "X";
			temp[0] = (*(path_t));
			append_to_dstring(extension_result, temp);
		}
		else if((*(path_t)) == '.')
		{
			in_extension = 1;
		}
		path_t++;
	}
}