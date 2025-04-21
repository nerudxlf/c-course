#include <cjson/cJSON.h>
#include <curl/curl.h>
#include <stdlib.h>
#include <string.h>

typedef char* string;

typedef struct {
  string data;
  size_t capacity;
  size_t length;
} string_buffer;

typedef struct {
  string temp_c;
  string description;
  string wind_speed;
  string wind_direction;
} wttr;

const string base_url = "https://wttr.in/";
const string query_param = "?format=j1";
const int base_url_len = strlen(base_url);
const int query_param_len = strlen(query_param);

size_t write_callback(void *ptr, size_t size, size_t nmemb, void *data) {
  size_t total_size = size * nmemb;
  string_buffer *buffer = (string_buffer *)data;
  if (buffer->length + total_size >= buffer->capacity) {
    printf("Buffer overflow!\n");
    return 0;
  }
  memcpy(buffer->data + buffer->length, ptr, total_size);
  buffer->length += total_size;
  buffer->data[buffer->length] = '\0';
  return total_size;
}

void clean_wttr(wttr *data) {
  free(data->temp_c);
  free(data->description);
  free(data->wind_direction);
  free(data->wind_speed);
  free(data);
}

void wttr_pprintln(wttr *data) {
  printf(
      "Temperature: %s\nDescription: %s\nWind direction: %s\nWind speed: %s\n",
      data->temp_c, data->description, data->wind_direction, data->wind_speed);
}

string get_string_safe(cJSON *obj, string key) {
  cJSON *item = cJSON_GetObjectItemCaseSensitive(obj, key);
  return (cJSON_IsString(item) && item->valuestring) ? item->valuestring : NULL;
}

wttr *get_wttr_from_json(string buffer) {
  cJSON *json = cJSON_Parse(buffer);
  if (json == NULL) {
    return NULL;
  }
  wttr *wttr_result = malloc(sizeof(wttr));
  if (wttr_result == NULL) {
    cJSON_Delete(json);
    return NULL;
  }
  cJSON *current_condition =
      cJSON_GetObjectItemCaseSensitive(json, "current_condition");
  if (current_condition == NULL) {
    free(wttr_result);
    cJSON_Delete(json);
    return NULL;
  }
  cJSON *current_condition_first = cJSON_GetArrayItem(current_condition, 0);
  if (current_condition_first == NULL) {
    free(wttr_result);
    cJSON_Delete(json);
    return NULL;
  }
  cJSON *description =
      cJSON_GetObjectItemCaseSensitive(current_condition_first, "weatherDesc");
  if (description == NULL) {
    free(wttr_result);
    cJSON_Delete(json);
    return NULL;
  }
  cJSON *description_first = cJSON_GetArrayItem(description, 0);
  if (description_first == NULL) {
    free(wttr_result);
    cJSON_Delete(json);
    return NULL;
  }

  string temp_C = get_string_safe(current_condition_first, "temp_C");
  if (temp_C == NULL) {
    free(wttr_result);
    cJSON_Delete(json);
    return NULL;
  }
  string windspeedKmph =
      get_string_safe(current_condition_first, "windspeedKmph");
  if (windspeedKmph == NULL) {
    free(wttr_result);
    cJSON_Delete(json);
    return NULL;
  }
  string winddir16Point =
      get_string_safe(current_condition_first, "winddir16Point");
  if (winddir16Point == NULL) {
    free(wttr_result);
    cJSON_Delete(json);
    return NULL;
  }
  string description_value = get_string_safe(description_first, "value");
  if (description_value == NULL) {
    printf("Error\n");
    free(wttr_result);
    cJSON_Delete(json);
    return NULL;
  }
  wttr_result->temp_c = strdup(temp_C);
  wttr_result->wind_speed = strdup(windspeedKmph);
  wttr_result->wind_direction = strdup(winddir16Point);
  wttr_result->description = strdup(description_value);
  cJSON_Delete(json);
  return wttr_result;
}

int get_api_wttr(CURL *curl, string city) {
  string_buffer buffer;
  buffer.capacity = 90000;
  buffer.length = 0;
  buffer.data = malloc(buffer.capacity);
  if (buffer.data == NULL) {
    return EXIT_FAILURE;
  }
  buffer.data[0] = '\0';
  string full_url = malloc(base_url_len + strlen(city) + query_param_len + 1);
  if (full_url == NULL) {
    free(buffer.data);
    return EXIT_FAILURE;
  }
  strcpy(full_url, base_url);
  strcat(full_url, city);
  strcat(full_url, query_param);
  curl_easy_setopt(curl, CURLOPT_URL, full_url);
  curl_easy_setopt(curl, CURLOPT_USERAGENT, "curl/7.68.0");
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buffer);
  CURLcode res = curl_easy_perform(curl);
  if (res != CURLE_OK) {
    printf("Ошибка: %s\n", curl_easy_strerror(res));
  } else {
    wttr *wttr_result = get_wttr_from_json(buffer.data);
    if (wttr_result == NULL) {
      printf("Something went wrong when parsing json");
    } else {
      wttr_pprintln(wttr_result);
      clean_wttr(wttr_result);
    }
  }
  free(full_url);
  free(buffer.data);
  return 0;
}

int main(int argc, char **argv) {
  if (argc != 2) {
    printf("USAGE: %s <file>\n", argv[0]);
    return EXIT_FAILURE;
  }
  string city = argv[1];
  CURL *curl = curl_easy_init();
  if (curl == NULL) {
    printf("Something went wrong when curl init");
    return EXIT_FAILURE;
  }
  printf("City: %s\n", city);
  get_api_wttr(curl, city);
  curl_easy_cleanup(curl);
}
