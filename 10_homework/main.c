#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <pthread.h>
#include <dirent.h>
#include <string.h>

typedef struct hash_entry {
    char* key;
    int value;
    struct hash_entry* next;
} hash_entry;

typedef struct {
    hash_entry** buckets;
    size_t capacity;
    size_t size;
    size_t load_factor_threshold;
} custom_map;

typedef struct {
    char** filenames; 
    size_t file_count;
    long offset;
    long limit;
    custom_map* urls;
    custom_map* referes;
    long total_bytes;
} thread_arg;

typedef struct {
    long total_bytes;
    char **top_urls;
    char **top_referes;
    size_t top_urls_size;
    size_t top_referes_size;
} file_statistics;

uint64_t fnv1a64(const char *s) {
    uint64_t hash = 0xcbf29ce484222325ULL;
    while (*s) {
        hash ^= (unsigned char)(*s++);
        hash *= 0x100000001b3ULL;
    }
    return hash;
}

char** get_top_values_from_map(custom_map* map, size_t value) {
    char** top_values = malloc(sizeof(char*) * value);
    
    char** used_keys = malloc(sizeof(char*) * value);
    size_t used_count = 0;
    
    for (size_t i = 0; i < value; i++){
        int current_max_value = 0;
        char* best_key = NULL;
        
        for (size_t j = 0; j < map->capacity; j++){
            hash_entry* entry = map->buckets[j];
            while (entry) {
                int already_used = 0;
                for (size_t k = 0; k < used_count; k++) {
                    if (strcmp(entry->key, used_keys[k]) == 0) {
                        already_used = 1;
                        break;
                    }
                }
                
                if (!already_used && entry->value > current_max_value) {
                    current_max_value = entry->value;
                    best_key = entry->key;
                }
                entry = entry->next;
            }
        }
        
        if (best_key) {
            top_values[i] = strdup(best_key);
            used_keys[used_count++] = best_key;
        } else {
            top_values[i] = strdup("(none)");
        }
    }
    
    free(used_keys);
    return top_values;
}

custom_map* create_map(void) {
    size_t capacity = 1024;
    custom_map* map = malloc(sizeof(custom_map));
    map->buckets = calloc(capacity, sizeof(hash_entry*));
    map->capacity = capacity;
    map->size = 0;
    map->load_factor_threshold = capacity * 3 / 4;
    return map;
}

static void free_bucket_chain(hash_entry* entry) {
    while (entry) {
        hash_entry* next = entry->next;
        free(entry->key);
        free(entry);
        entry = next;
    }
}

void custom_map_free(custom_map* map) {
    for (size_t i = 0; i < map->capacity; i++) {
        free_bucket_chain(map->buckets[i]);
    }
    free(map->buckets);
    free(map);
}

static size_t hash_index(const char* key, size_t capacity) {
    uint64_t hash = fnv1a64(key);
    return hash & (capacity - 1);
}

static void resize_map(custom_map* map) {
    size_t old_capacity = map->capacity;
    hash_entry** old_buckets = map->buckets;
    
    map->capacity *= 2;
    map->load_factor_threshold = map->capacity * 3 / 4;
    map->buckets = calloc(map->capacity, sizeof(hash_entry*));
    map->size = 0;
    
    for (size_t i = 0; i < old_capacity; i++) {
        hash_entry* entry = old_buckets[i];
        while (entry) {
            hash_entry* next = entry->next;
            size_t new_index = hash_index(entry->key, map->capacity);
            entry->next = map->buckets[new_index];
            map->buckets[new_index] = entry;
            map->size++;
            entry = next;
        }
    }
    
    free(old_buckets);
}

void custom_map_set(custom_map* map, char* key, int value) {
    size_t index = hash_index(key, map->capacity);
    hash_entry* entry = map->buckets[index];
    
    while (entry) {
        if (strcmp(entry->key, key) == 0) {
            entry->value = value;
            return;
        }
        entry = entry->next;
    }
    
    hash_entry* new_entry = malloc(sizeof(hash_entry));
    new_entry->key = strdup(key);
    new_entry->value = value;
    new_entry->next = map->buckets[index];
    map->buckets[index] = new_entry;
    map->size++;
    
    if (map->size > map->load_factor_threshold) {
        resize_map(map);
    }
}

int custom_map_get(custom_map* map, char* key) {
    size_t index = hash_index(key, map->capacity);
    hash_entry* entry = map->buckets[index];
    
    while (entry) {
        if (strcmp(entry->key, key) == 0) {
            return entry->value;
        }
        entry = entry->next;
    }
    return 0;
}

custom_map* join_custom_maps(custom_map** maps, size_t count) {
    custom_map* result = create_map();
    for (size_t i = 0; i < count; i++){
        for (size_t j = 0; j < maps[i]->capacity; j++){
            hash_entry* entry = maps[i]->buckets[j];
            while (entry) {
                custom_map_set(
                    result,
                    entry->key,
                    entry->value + custom_map_get(result, entry->key)
                );
                entry = entry->next;
            }
        }
    }
    return result;
}

char* url_decode(const char* encoded) {
    if (!encoded) return NULL;
    
    size_t len = strlen(encoded);
    char* decoded = malloc(len + 1);
    if (!decoded) return NULL;
    
    size_t i = 0, j = 0;
    while (i < len) {
        if (encoded[i] == '%' && i + 2 < len) {
            int hex_value;
            if (sscanf(&encoded[i + 1], "%2x", &hex_value) == 1) {
                decoded[j++] = (char)hex_value;
                i += 3;
            } else {
                decoded[j++] = encoded[i++];
            }
        } else if (encoded[i] == '+') {
            decoded[j++] = ' ';
            i++;
        } else {
            decoded[j++] = encoded[i++];
        }
    }
    decoded[j] = '\0';
    return decoded;
}

char* get_url(const char *line) {
    char *start = strstr(line, "\"");
    if (!start) return NULL;
    start++;
    
    char *space1 = strchr(start, ' ');
    if (!space1) return NULL;
    space1++;
    
    char *space2 = strchr(space1, ' ');
    if (!space2) return NULL;
    
    size_t len = space2 - space1;
    char *encoded_url = malloc(len + 1);
    if (!encoded_url) return NULL;
    
    strncpy(encoded_url, space1, len);
    encoded_url[len] = '\0';
    
    char *decoded_url = url_decode(encoded_url);
    free(encoded_url);
    
    return decoded_url;
}

long get_bytes(const char *line) {
    char *quote_end = strstr(line, "\" ");
    if (!quote_end) return -1;
    quote_end += 2;
    
    char *space = strchr(quote_end, ' ');
    if (!space) return -1;
    space++;
    
    if (*space == '-') return 0;
    return strtol(space, NULL, 10);
}

char* get_referer(const char *line) {
    char *quote_end = strstr(line, "\" ");
    if (!quote_end) return NULL;
    quote_end += 2;
    
    char *space = strchr(quote_end, ' ');
    if (!space) return NULL;
    space++;
    
    space = strchr(space, ' ');
    if (!space) return NULL;
    space++;
    
    if (*space != '"') return NULL;
    space++;
    
    char *ref_end = strchr(space, '"');
    if (!ref_end) return NULL;
    
    size_t len = ref_end - space;
    char *referer = malloc(len + 1);
    if (!referer) return NULL;
    
    strncpy(referer, space, len);
    referer[len] = '\0';
    
    if (strcmp(referer, "-") == 0) {
        free(referer);
        return strdup("(no referer)");
    }
    
    char *decoded_referer = url_decode(referer);
    free(referer);
    
    return decoded_referer;
}

void* thread_func(void* arg) {
    long total_bytes = 0;
    thread_arg* args = (thread_arg*)arg;
    
    for (size_t f = 0; f < args->file_count; f++) {
        FILE* file = fopen(args->filenames[f], "r");
        if (file == NULL) {
            perror("fopen");
            continue;
        }
        fseek(file, args->offset, SEEK_SET);
        char* line = NULL;
        size_t line_len = 0;
        ssize_t read;
        while ((read = getline(&line, &line_len, file)) != -1) {
            char* url = get_url(line);
            char* refer = get_referer(line);
            if (url == NULL || refer == NULL) continue;
            long bytes = get_bytes(line);
            if (bytes == -1) continue;
            total_bytes += bytes;
            custom_map_set(args->urls, url, custom_map_get(args->urls, url) + bytes);
            custom_map_set(args->referes, refer, custom_map_get(args->referes, refer) + bytes);
        }
        fclose(file);
    }
    args->total_bytes = total_bytes;
    return NULL;
}

char** pathes_to_files(const char* logs_dir, size_t* out_count) {
    DIR *dir = opendir(logs_dir);
    if (dir == NULL){
        perror("opendir");
        return NULL;
    }

    struct dirent *entry;
    char** pathes = malloc(sizeof(char*) * 100);
    size_t i = 0;
    while ((entry = readdir(dir)) != NULL){
        if (entry->d_name[0] == '.' && (entry->d_name[1] == '\0' || (entry->d_name[1] == '.' && entry->d_name[2] == '\0')))
            continue;
        pathes[i++] = strdup(entry->d_name);

    }
    closedir(dir);
    if (out_count) *out_count = i;
    return pathes;
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <threads> <logs_dir>\n", argv[0]);
        return 1;
    }
    char *endptr = NULL;
    long threads = strtol(argv[1], &endptr, 10);
    const char* logs_dir = argv[2];
    size_t files_count = 0;
    char** pathes = pathes_to_files(logs_dir, &files_count);
    if (pathes == NULL){
        fprintf(stderr, "Error");
        return 1;
    }
    if (*endptr != '\0' || threads <= 0) {
        fprintf(stderr, "Invalid threads value: %s\n", argv[1]);
        return 1;
    }
    
    size_t num_threads = (size_t)threads;
    if (num_threads > files_count) {
        num_threads = files_count;
    }
    
    pthread_t* threads_arr = malloc(sizeof(pthread_t) * num_threads);
    thread_arg** args = malloc(sizeof(thread_arg*) * num_threads);
    
    size_t dir_len = strlen(logs_dir);
    int needs_slash = (logs_dir[dir_len - 1] != '/');
    size_t files_per_thread = files_count / num_threads;
    size_t extra_files = files_count % num_threads;
    size_t current_file_index = 0;

    for (size_t i = 0; i < num_threads; i++) {
        args[i] = malloc(sizeof(thread_arg));
        size_t files_for_this_thread = files_per_thread + (i < extra_files ? 1 : 0);
        args[i]->filenames = malloc(sizeof(char*) * files_for_this_thread);
        args[i]->file_count = files_for_this_thread;
        
        for (size_t f = 0; f < files_for_this_thread; f++) {
            size_t file_index = current_file_index + f;
            size_t full_path_len = dir_len + strlen(pathes[file_index]) + (needs_slash ? 2 : 1);
            args[i]->filenames[f] = malloc(full_path_len);
            if (needs_slash) {
                snprintf(args[i]->filenames[f], full_path_len, "%s/%s", logs_dir, pathes[file_index]);
            } else {
                snprintf(args[i]->filenames[f], full_path_len, "%s%s", logs_dir, pathes[file_index]);
            }
        }
        current_file_index += files_for_this_thread;
        
        args[i]->offset = 0;
        args[i]->limit = 0;
        args[i]->urls = create_map();
        args[i]->referes = create_map();
        pthread_create(&threads_arr[i], NULL, thread_func, args[i]);
    }
    
    
    for (size_t i = 0; i < num_threads; i++){
        pthread_join(threads_arr[i], NULL);
    }
    
    custom_map** url_maps = malloc(sizeof(custom_map*) * num_threads);
    custom_map** refer_maps = malloc(sizeof(custom_map*) * num_threads);
    long total_bytes = 0;
    for (size_t i = 0; i < num_threads; i++){
        url_maps[i] = args[i]->urls;
        refer_maps[i] = args[i]->referes;
        total_bytes += args[i]->total_bytes;
    }
    custom_map* urls = join_custom_maps(url_maps, num_threads);
    custom_map* referes = join_custom_maps(refer_maps, num_threads);
    printf("Total bytes: %ld\n", total_bytes);
    char** top_urls = get_top_values_from_map(urls, 10);
    char** top_referes = get_top_values_from_map(referes, 10);
    for (size_t i = 0; i < 10; i++){
        printf("Top URL: %s\n", top_urls[i]);
        printf("Top Referer: %s\n", top_referes[i]);
    }
    
    free(top_urls);
    free(top_referes);
    custom_map_free(urls);
    custom_map_free(referes);
    for (size_t i = 0; i < num_threads; i++) {
        for (size_t f = 0; f < args[i]->file_count; f++) {
            free(args[i]->filenames[f]);
        }
        free(args[i]->filenames);
        free(args[i]);
    }
    free(args);
    free(threads_arr);
    free(url_maps);
    free(refer_maps);
    for (size_t i = 0; i < files_count; i++) {
        free(pathes[i]);
    }
    free(pathes);
    
    return 0;
}
